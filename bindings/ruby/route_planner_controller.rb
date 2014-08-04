require 'mmspa4pg_ffi'

class RoutePlannerController < ApplicationController
  def index
    render :layout => false
  end

  def do_routing
    ################### Input parameters processing ###################
    datasource = params[:datasource]
    ori_start = params[:start_coordinate].split(/,/)
    ori_end = params[:end_coordinate].split(/,/)
    private_car_available = params[:private_car_available]
    need_parking = params[:need_parking]
    can_use_public = params[:can_use_public_transit]
    suburban = params[:suburban]
    underground = params[:underground]
    tram = params[:tram]
    bus = params[:bus]
    objective = params[:objective]
    @final_start = StreetJunctions.find_nearest(Point.from_x_y(ori_start[1].to_f, ori_start[0].to_f), 0.005)    
    @final_end = StreetJunctions.find_nearest(Point.from_x_y(ori_end[1].to_f, ori_end[0].to_f), 0.005)
    if @final_start != nil and @final_end != nil then
      start_id = @final_start.nodeid
      end_id = @final_end.nodeid

      options = InferenceEngine::RoutingOptions.new
      options.objective = objective
      if private_car_available == 'yes' then
        options.has_private_car = true
      end
      if need_parking == 'yes' then
        options.need_parking = true
      end
      if can_use_public == 'yes' then
        options.can_use_public = true
        if suburban == 'yes' then
          options.can_use_suburban(true)
        end
        if underground == 'yes' then
          options.can_use_underground(true)
        end
        if tram == 'yes' then
          options.can_use_tram(true)
        end
        #        if bus == 'yes' then
        #          options.can_use_bus(true)
        #        end
      end

      ################### Routing plans inference and construction ###################
      @routing_plan_list = InferenceEngine::RoutingPlanInferer.generate_routing_plan(options)
      @result_list = []
      threads = []
      cached_result = RoutingResultCache.find_by_source_and_target_and_option_id(start_id, end_id, options.generate_id)
      if cached_result == nil then
        if (Mmspa.ConnectDB("dbname = multimodal_munich user = liulu password = workhard") == 1) then
          render :partial => 'data_error'
          RAILS_DEFAULT_LOGGER.info "connect to database error"
        else
          ################### Path finding according to every possible routing plan ###################
          0.upto(@routing_plan_list.length - 1) do |k|
            #threads << Thread.new(@routing_plan_list[k]) do |plan|
            plan = @routing_plan_list[k]
            RAILS_DEFAULT_LOGGER.info "---- Route calculation thread #{k} start... ----"
            mode_count = plan.mode_list.length
            modes = plan.mode_list
            public_mode_count = 0
            public_modes = []
            if plan.has_public_transit then
              public_mode_count = plan.public_transit_set.length
              public_modes = plan.public_transit_set
              RAILS_DEFAULT_LOGGER.info "public transit mode count: #{public_mode_count}"
            end
            switch_conditions = plan.switch_condition_list
            RAILS_DEFAULT_LOGGER.info "Create routing plan..."
            Mmspa.CreateRoutingPlan(mode_count, public_mode_count)
            0.upto(mode_count - 1) do |i|
              Mmspa.SetModeListItem(i, modes[i])
            end
            0.upto(mode_count - 2) do |i|
              Mmspa.SetSwitchConditionListItem(i, switch_conditions[i])
              Mmspa.SetSwitchingConstraint(i, plan.switch_constraint_list[i])
            end
            if plan.has_public_transit then
              0.upto(public_mode_count - 1) do |i|
                Mmspa.SetPublicTransitModeSetItem(i, public_modes[i])
              end
            end
            Mmspa.SetTargetConstraint(plan.target_constraint)
            Mmspa.SetCostFactor(plan.cost_factor)
            source_vertices = target_vertices = []
            source_vertices = Vertex.find(:all, :conditions => ["vertex_id % 100000000 = :start_id", {:start_id => start_id}])
            target_vertices = Vertex.find(:all, :conditions => ["vertex_id % 100000000 = :end_id", {:end_id => end_id}])
            RAILS_DEFAULT_LOGGER.info "source id: #{source_vertices}"
            RAILS_DEFAULT_LOGGER.info "target id: #{target_vertices}"
            result = Mmspa::RoutingResult.new
            result.planned_mode_list = modes
            result.description = plan.description
            result.planned_switch_type_list = plan.switch_type_list
      
            source = target = -1
            0.upto(source_vertices.length - 1) do |m|
              0.upto(target_vertices.length - 1) do |n|
                if Mmspa::MultimodalRoutePlanner.input_valid?(modes[0], modes[-1], source_vertices[m].vertex_id, target_vertices[n].vertex_id, public_modes) then
                  source = source_vertices[m].vertex_id
                  target = target_vertices[n].vertex_id
                  break
                end
              end
              if source != -1 and target != -1 then
                break
              end
            end
      
            if source != -1 and target != -1 then
              start_time = Time.now
              RAILS_DEFAULT_LOGGER.info "parsing the data..."
              ################### Graph data reading ###################
              if Mmspa.Parse() == 1 then
                render :partial => 'data_error'
                #            Thread.exit
              else
                RAILS_DEFAULT_LOGGER.info "parsing successfully!"
                end_time = Time.now
                ellapsed = end_time - start_time
                RAILS_DEFAULT_LOGGER.info "time consumed by reading data: #{ellapsed.to_s} s"
                RAILS_DEFAULT_LOGGER.info "calculating route by MultimodalTwoQ..."
      
                start_time = Time.now
                ################### Shortest path calculation ###################
                Mmspa.MultimodalTwoQ(source)
                end_time = Time.now
                ellapsed = end_time - start_time
                RAILS_DEFAULT_LOGGER.info "time consumed by route calculation: #{ellapsed.to_s} s"
                paths_ptr = FFI::MemoryPointer.new(:pointer, mode_count)
                paths_ptr = Mmspa.GetFinalPath(source, target)
                if (paths_ptr.null?) then
                  RAILS_DEFAULT_LOGGER.info "no path found"
                  result.is_existent = false
                else
                  paths = []
                  vertex_count = 0
                  # get final paths from the marshaled pointer
                  mode_count.times do |i|
                    paths[i] = Mmspa::Path.new(paths_ptr.get_pointer(i * 4))
                    RAILS_DEFAULT_LOGGER.info "#{modes[i]}"
                    RAILS_DEFAULT_LOGGER.info "#{paths[i][:vertex_list_length]}"
                    result.paths_by_vertex_id[i] = []
                    0.upto(paths[i][:vertex_list_length] - 1) do |j|
                      result.paths_by_vertex_id[i][j] = paths[i][:vertex_list].get_int64(j * 8)
                    end
                    vertex_count += result.paths_by_vertex_id[i].length
                  end
                  if vertex_count == 0 then
                    result.is_existent = false
                  else
                    result.is_existent = true
                    result.length = Mmspa.GetFinalCost(target, 'distance')
                    RAILS_DEFAULT_LOGGER.info "total distance: #{result.length}"
                    result.time = Mmspa.GetFinalCost(target, 'elapsed_time')
                    RAILS_DEFAULT_LOGGER.info "total time: #{result.time}"
                    result.walking_length = Mmspa.GetFinalCost(target, 'walking_distance')
                    RAILS_DEFAULT_LOGGER.info "total walking distance: #{result.walking_length}"
                    result.walking_time = Mmspa.GetFinalCost(target, 'walking_time')
                    RAILS_DEFAULT_LOGGER.info "total walking time: #{result.walking_time}"
                  end
                  RAILS_DEFAULT_LOGGER.info "Release final paths generated by mmspa4pg"
                  Mmspa.DisposePaths(paths_ptr)
                end
                RAILS_DEFAULT_LOGGER.info "Release memory used by mmspa4pg"
                Mmspa.Dispose()
              end
            else
              result.is_existent = false
            end
            @result_list.push(result)
            RAILS_DEFAULT_LOGGER.info "---- Route calculation thread #{k} finish ----"
            #end # end of the calculation thread
          end
        end
        Mmspa.DisconnectDB()

      #threads.each { |t| t.join  }

        ################### Deduplicate the routing results ###################
        0.upto(@result_list.length - 2) do |i|
          (i + 1).upto(@result_list.length - 1) do |j|
            if (@result_list[i].is_existent == true) and (@result_list[j].is_existent == true) then
              if (@result_list[i].paths_by_vertex_id == @result_list[j].paths_by_vertex_id) then
                if (@result_list[i].planned_mode_list.include?(1900)) then
                  @result_list[i].is_existent = false
                elsif (@result_list[j].planned_mode_list.include?(1900)) then
                  @result_list[j].is_existent = false
                elsif (@result_list[i].planned_mode_list.include?(1002)) then
                  @result_list[i].is_existent = false
                elsif (@result_list[j].planned_mode_list.include?(1002)) then
                  @result_list[j].is_existent = false
                end
              end
            end
          end
        end

        ################### Remove (set is_existent = false) the path containing pure walking path in 1900 mode ###################
        pt_modes = [1003, 1004, 1005, 1006]
        0.upto(@result_list.length - 1) do |i|
          if (@result_list[i].is_existent == true) and (@result_list[i].planned_mode_list.include?(1900)) then
            pt_vertex_counter = 0
            @result_list[i].paths_by_vertex_id.each do |p|
              p.each do |v|
                if pt_modes.include?(Vertex.find_by_vertex_id(v).mode_id) then
                  pt_vertex_counter += 1
                  #              if pt_vertex_counter >= 2 then
                  #                break
                  #              end
                end
              end
            end
            RAILS_DEFAULT_LOGGER.info "pt_vertex count: #{pt_vertex_counter}"
            if pt_vertex_counter < 2 then
              @result_list[i].is_existent = false
            end
          end
        end

        ################### Final paths construction for visualization ###################
        0.upto(@result_list.length - 1) do |k|
          modes = @result_list[k].planned_mode_list
          mode_count = modes.length
          if @result_list[k].is_existent == true then
            # If there is no PT involved in the route calculation,
            # the mode list for rendering will be the same as the one for calculation.
            # Otherwise, the PT mode whose id is 1900 should be divided into several parts for visualization.
            if !(modes.include?(1900)) then
              @result_list[k].rendering_mode_list = modes
            else
              get_rendering_mode_list(k)
            end
            paths_by_linestrings = []
            0.upto(@result_list[k].rendering_mode_list.length - 1) do |i|
              paths_by_linestrings[i] = []
              @result_list[k].paths_by_link_id[i] = []
            end
            @result_list[k].is_existent = true
            @result_list[k].paths_by_points = []

            RAILS_DEFAULT_LOGGER.info "number of modes for rendering #{@result_list[k].rendering_mode_list.length}"
            rendering_mode_cursor = 0
            0.upto(mode_count - 1) do |i|
              RAILS_DEFAULT_LOGGER.info "creating path in line segment strings for mode #{@result_list[k].planned_mode_list[i]}"
              RAILS_DEFAULT_LOGGER.info "rendering_mode_cursor: #{rendering_mode_cursor}"
              case modes[i]
              when 1001, 1002
                # find the features in road network layer -- street_lines table, StreetLines model class
                RAILS_DEFAULT_LOGGER.info "Find features in street_lines..."
                0.upto(@result_list[k].paths_by_vertex_id[i].length - 2) do |j|
                  street_seg = StreetLines.find(:first, :conditions => [
                      "(fnodeid = :from_id AND tnodeid = :to_id) OR (fnodeid = :to_id AND tnodeid = :from_id)",
                      {:from_id => @result_list[k].paths_by_vertex_id[i][j] % 100000000,
                        :to_id => @result_list[k].paths_by_vertex_id[i][j + 1] % 100000000}
                    ])
                  paths_by_linestrings[rendering_mode_cursor][j] = street_seg.the_geom
                  edge = Edge.find_by_from_id_and_to_id_and_mode_id(@result_list[k].paths_by_vertex_id[i][j],
                    @result_list[k].paths_by_vertex_id[i][j + 1], modes[i])
                  @result_list[k].paths_by_link_id[rendering_mode_cursor][j] = edge.edge_id / 100
                end
                rendering_mode_cursor += 1
                RAILS_DEFAULT_LOGGER.info "rendering_mode_cursor: #{rendering_mode_cursor}"
              when 1900
                # find the features in public transit system --
                # street_lines table, StreetLines model class for pedestrian path segment
                # suburban_lines table, SuburbanLines model class for S-bahn
                # underground_lines table, UndergroundLines model class for U-bahn
                # tram_lines table, TramLines model class for Tram
                RAILS_DEFAULT_LOGGER.info "Find features in public transit layers..."
                0.upto(@result_list[k].paths_by_vertex_id[i].length - 2) do |j|
                  from_mode_id = Vertex.find_by_vertex_id(@result_list[k].paths_by_vertex_id[i][j]).mode_id
                  to_mode_id = Vertex.find_by_vertex_id(@result_list[k].paths_by_vertex_id[i][j + 1]).mode_id
                  if  from_mode_id != to_mode_id then
                    # mode change, there must be a inner-PT switch point here
                    switch_point = SwitchPoint.find_by_from_vertex_id_and_to_vertex_id_and_from_mode_id_and_to_mode_id(
                      @result_list[k].paths_by_vertex_id[i][j],
                      @result_list[k].paths_by_vertex_id[i][j + 1],
                      from_mode_id,
                      to_mode_id)
                    @result_list[k].switch_type_list.push(switch_point.type_id)
                    switch_point_poi = nil
                    case switch_point.type_id
                    when 2004
                      # underground_station
                      switch_point_poi = UndergroundStations.find_by_type_id(switch_point.ref_poi_id)
                    when 2005
                      # suburban_station
                      switch_point_poi = SuburbanStations.find_by_type_id(switch_point.ref_poi_id)
                    when 2006
                      # tram_station
                      switch_point_poi = TramStations.find_by_type_id(switch_point.ref_poi_id)
                    when 2007
                      # bus_station
                      # no bus lines so far
                    end
                    @result_list[k].switch_point_list.push(switch_point_poi.the_geom)
                    @result_list[k].switch_point_name_list.push(switch_point_poi.um_name)
                    rendering_mode_cursor += 1
                    RAILS_DEFAULT_LOGGER.info "rendering_mode_cursor: #{rendering_mode_cursor}"
                  else
                    case from_mode_id
                    when 1002
                      public_seg = StreetLines.find(:first, :conditions => [
                          "(fnodeid = :from_id AND tnodeid = :to_id) OR (fnodeid = :to_id AND tnodeid = :from_id)",
                          {:from_id => @result_list[k].paths_by_vertex_id[i][j] % 100000000,
                            :to_id => @result_list[k].paths_by_vertex_id[i][j + 1] % 100000000}
                        ])
                      paths_by_linestrings[rendering_mode_cursor].push(public_seg.the_geom)
                    when 1003
                      # underground
                      public_seg = UndergroundLines.find(:first, :conditions => [
                          "(fnodeid = :from_id AND tnodeid = :to_id) OR (fnodeid = :to_id AND tnodeid = :from_id)",
                          {:from_id => @result_list[k].paths_by_vertex_id[i][j] % 100000000,
                            :to_id => @result_list[k].paths_by_vertex_id[i][j + 1] % 100000000}
                        ])
                      paths_by_linestrings[rendering_mode_cursor].push(public_seg.the_geom)
                      RAILS_DEFAULT_LOGGER.info "underground segment geom: #{paths_by_linestrings[rendering_mode_cursor][-1]}"
                    when 1004
                      # suburban
                      public_seg = SuburbanLines.find(:first, :conditions => [
                          "(fnodeid = :from_id AND tnodeid = :to_id) OR (fnodeid = :to_id AND tnodeid = :from_id)",
                          {:from_id => @result_list[k].paths_by_vertex_id[i][j] % 100000000,
                            :to_id => @result_list[k].paths_by_vertex_id[i][j + 1] % 100000000}
                        ])
                      paths_by_linestrings[rendering_mode_cursor].push(public_seg.the_geom)
                      RAILS_DEFAULT_LOGGER.info "suburban segment geom: #{paths_by_linestrings[rendering_mode_cursor][-1]}"
                    when 1005
                      # tram
                      public_seg = TramLines.find(:first, :conditions => [
                          "(fnodeid = :from_id AND tnodeid = :to_id) OR (fnodeid = :to_id AND tnodeid = :from_id)",
                          {:from_id => @result_list[k].paths_by_vertex_id[i][j] % 100000000,
                            :to_id => @result_list[k].paths_by_vertex_id[i][j + 1] % 100000000}
                        ])
                      paths_by_linestrings[rendering_mode_cursor].push(public_seg.the_geom)
                      RAILS_DEFAULT_LOGGER.info "tram segment geom: #{paths_by_linestrings[rendering_mode_cursor][-1]}"
                    end
                    edge = Edge.find_by_from_id_and_to_id_and_mode_id(@result_list[k].paths_by_vertex_id[i][j],
                      @result_list[k].paths_by_vertex_id[i][j + 1], from_mode_id)
                    @result_list[k].paths_by_link_id[rendering_mode_cursor].push(edge.edge_id / 100)
                  end
                end
              else
                RAILS_DEFAULT_LOGGER.info "Find features in some other layers NOT supported yet..."
              end
              if (i < mode_count - 1) then
                switch_point = SwitchPoint.find_by_from_vertex_id_and_to_vertex_id_and_from_mode_id_and_to_mode_id_and_type_id(
                  @result_list[k].paths_by_vertex_id[i][-1],
                  @result_list[k].paths_by_vertex_id[i + 1][0],
                  @result_list[k].paths_by_vertex_id[i][-1] / 100000000,
                  @result_list[k].paths_by_vertex_id[i + 1][0] / 100000000,
                  @result_list[k].planned_switch_type_list[i])
                @result_list[k].switch_type_list.push(switch_point.type_id)
                switch_point_poi = nil
                switch_point_poi_name = ""
                case @result_list[k].planned_switch_type_list[i]
                when 2001
                  # parking lot
                  switch_point_poi = CarParking.find_by_um_id(switch_point.ref_poi_id)
                  switch_point_poi_name = switch_point_poi.poi_name
                when 2002
                  # geo_connection
                  switch_point_poi = StreetJunctions.find_by_nodeid(switch_point.from_vertex_id % 100000000)
                  switch_point_poi_name = ""
                when 2003
                  # park and ride
                  switch_point_poi = ParkAndRide.find_by_poi_id(switch_point.ref_poi_id)
                  switch_point_poi_name = switch_point_poi.um_name
                when 2008
                  # kiss and ride
                  case switch_point.to_mode_id
                  when 1003
                    # underground_station
                    switch_point_poi = UndergroundStations.find_by_type_id(switch_point.ref_poi_id)
                  when 1004
                    # suburban_station
                    switch_point_poi = SuburbanStations.find_by_type_id(switch_point.ref_poi_id)
                  when 1005
                    # tram_station
                    switch_point_poi = TramStations.find_by_type_id(switch_point.ref_poi_id)
                  end
                  switch_point_poi_name = switch_point_poi.um_name
                end
                @result_list[k].switch_point_list.push(switch_point_poi.the_geom)
                @result_list[k].switch_point_name_list.push(switch_point_poi_name)
              end
            end

            RAILS_DEFAULT_LOGGER.info "number of modes for rendering #{@result_list[k].rendering_mode_list.length}"
            0.upto(@result_list[k].rendering_mode_list.length - 1) do |i|
              RAILS_DEFAULT_LOGGER.info "creating path in point coordinates for mode #{@result_list[k].rendering_mode_list[i]}"
              if paths_by_linestrings[i].empty? then
                @result_list[k].paths_by_points[i] = []
              else
                RAILS_DEFAULT_LOGGER.info "paths_by_linestrings[i][0] #{paths_by_linestrings[i][0]}"
                if (paths_by_linestrings[i].length == 1) then
                  @result_list[k].paths_by_points[i] = paths_by_linestrings[i][0].geometries[0].points
                else
                  threshold = 1.0e-6
                  if (get_deviation(paths_by_linestrings[i][0].geometries[0][0], paths_by_linestrings[i][1].geometries[0][0]) < threshold) ||
                      (get_deviation(paths_by_linestrings[i][0].geometries[0][0], paths_by_linestrings[i][1].geometries[0][-1]) < threshold) then
                    @result_list[k].paths_by_points[i] = paths_by_linestrings[i][0].geometries[0].points.reverse
                  else
                    @result_list[k].paths_by_points[i] = paths_by_linestrings[i][0].geometries[0].points
                  end
                  1.upto(paths_by_linestrings[i].length - 1) do |j|
                    if (get_deviation(paths_by_linestrings[i][j].geometries[0][-1], @result_list[k].paths_by_points[i][-1]) < threshold) then
                      @result_list[k].paths_by_points[i] += paths_by_linestrings[i][j].geometries[0].points.reverse
                    else
                      @result_list[k].paths_by_points[i] += paths_by_linestrings[i][j].geometries[0].points
                    end
                  end
                end
              end
            end
          end
        end

        ################### Sort the routing results according to the travel time ###################
        0.upto(@result_list.length - 2) do |i|
          (i + 1).upto(@result_list.length - 1) do |j|
            if (@result_list[i].is_existent == true) and (@result_list[j].is_existent == true) then
              if (@result_list[i].time > @result_list[j].time) then
                @result_list[i], @result_list[j] = @result_list[j], @result_list[i]
              end
            end
          end
        end
        RoutingResultCache.create(:source => start_id, :target => end_id, :option_id => options.generate_id, :result_list => @result_list)
      else
        @result_list = cached_result.result_list
      end
      render :partial => 'results'
    else
      render :partial => 'data_error'
    end
  end

  def get_deviation(p1, p2)
    x_deviation = (p1.x - p2.x).abs
    y_deviation = (p1.y - p2.y).abs
    (x_deviation + y_deviation) / 2
  end

  def get_rendering_mode_list(planIndex)
    0.upto(@result_list[planIndex].planned_mode_list.length - 1) do |i|
      if @result_list[planIndex].planned_mode_list[i] != 1900 then
        @result_list[planIndex].rendering_mode_list.push(@result_list[planIndex].planned_mode_list[i])
        RAILS_DEFAULT_LOGGER.info "mode #{@result_list[planIndex].planned_mode_list[i]} for rendering"
        RAILS_DEFAULT_LOGGER.info "number of modes for rendering #{@result_list[planIndex].rendering_mode_list.length}"
      else
        @result_list[planIndex].rendering_mode_list.push(Vertex.find_by_vertex_id(@result_list[planIndex].paths_by_vertex_id[i][0]).mode_id)
        RAILS_DEFAULT_LOGGER.info "mode #{@result_list[planIndex].rendering_mode_list[0]} for rendering"
        0.upto(@result_list[planIndex].paths_by_vertex_id[i].length - 2) do |j|
          from_mode_id = Vertex.find_by_vertex_id(@result_list[planIndex].paths_by_vertex_id[i][j]).mode_id
          to_mode_id = Vertex.find_by_vertex_id(@result_list[planIndex].paths_by_vertex_id[i][j + 1]).mode_id
          if from_mode_id != to_mode_id then
            @result_list[planIndex].rendering_mode_list.push(to_mode_id)
            RAILS_DEFAULT_LOGGER.info "mode #{to_mode_id} for rendering"
          end
        end
      end
    end
  end

end
