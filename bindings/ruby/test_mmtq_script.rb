#sc = Mmspa::SwitchCondition.new
#sc.type = 'geo_connection'
#plan = Mmspa::RoutingPlan.new
#plan.switch_condition_list[0] = sc
#plan.mode_list[0] = 'private_car'
#plan.mode_list[1] = 'foot'
#plan.is_multimodal = true
#plan.optimizing_field = 'time'
#router = Mmspa::MultimodalTwoQ.new(plan)
#router.search_path?(7579.0,6854.0)
#
#sc = Mmspa::SwitchCondition.new
#plan = Mmspa::RoutingPlan.new
#plan.mode_list[0] = 'private_car'
#plan.is_multimodal = false
#plan.optimizing_field = 'time'
#router = Mmspa::MultimodalTwoQ.new(plan)
#router.search_path?(7579.0,4744.0)

require 'rubygems'
require 'ffi'
require 'mmspa4pg_ffi'

options = InferenceEngine::RoutingOptions.new
options.objective = 'fastest'
options.has_private_car = false
options.need_parking = false
options.can_use_public = true
options.can_use_underground(true)
routing_plan_list = InferenceEngine::RoutingPlanInferer.generate_routing_plan(options)
plan = routing_plan_list[1]

mode_count = plan.mode_list.length
modes = plan.mode_list
public_mode_count = 0
public_modes = []
if plan.has_public_transit then
  public_mode_count = plan.public_transit_set.length
  public_modes = plan.public_transit_set
end
switch_conditions = plan.switch_condition_list
Mmspa.CreateRoutingPlan(mode_count, public_mode_count)
0.upto(mode_count - 1) do |i|
  Mmspa.SetModeListItem(i, modes[i])
end
0.upto(mode_count - 2) do |i|
  Mmspa.SetSwitchConditionListItem(i, switch_conditions[i])
end
if plan.has_public_transit then
  0.upto(public_mode_count - 1) do |i|
    Mmspa.SetPublicTransitModeSetItem(i, public_modes[i])
  end
end
Mmspa.SetCostFactor(plan.cost_factor)
source = 100201026474
target = 100201023383
paths_by_vertices = []
start_time = Time.now
Mmspa.Parse("dbname = multimodalrouting_development user = liulu password = workhard") == 1

end_time = Time.now
ellapsed = end_time - start_time
puts "time consumed by reading data: #{ellapsed.to_s} s"
puts "calculating route by MultimodalTwoQ..."
#  if Mmspa::MultimodalRoutePlanner.input_valid?("1001", "1002", source, target) then
start_time = Time.now
Mmspa.MultimodalTwoQ(source)
end_time = Time.now
ellapsed = end_time - start_time
puts "time consumed by route calculation: #{ellapsed.to_s} s"
paths_ptr = FFI::MemoryPointer.new(:pointer, mode_count)
paths_ptr = Mmspa.GetFinalPath(source, target)

paths = []  
0.upto(mode_count - 1) do |i|
  paths[i] = Mmspa::Path.new(paths_ptr.get_pointer(i * 4))
  puts "#{paths[i][:vertex_list_length]}"
  paths_by_vertices[i] = []
  paths_by_vertices[i] = paths[i][:vertex_list].read_array_of_long(paths[i][:vertex_list_length])

  0.upto(paths[i][:vertex_list_length] - 1) do |j|
    paths_by_vertices[i][j] = paths[i][:vertex_list].get_int64(j * 8)
  end
end

Mmspa.Dispose()
Mmspa.DisposePaths(paths_ptr)
#  else
#    puts "input invalid!"
#  end  

paths_by_linestrings = []
paths_by_points = []
rendering_mode_list = []

if !(modes.include?(1900)) then
  rendering_mode_list = modes
else
  0.upto(modes.length - 1) do |i|
    if modes[i] != 1900 then
      rendering_mode_list.push(modes[i])
    else
      rendering_mode_list.push(Vertex.find_by_vertex_id(paths_by_vertices[i][0]).mode_id)

      0.upto(paths_by_vertices[i].length - 2) do |j|
        from_mode_id = Vertex.find_by_vertex_id(paths_by_vertices[i][j]).mode_id
        to_mode_id = Vertex.find_by_vertex_id(paths_by_vertices[i][j + 1]).mode_id
        if from_mode_id != to_mode_id then
          rendering_mode_list.push(to_mode_id)
        end
      end
    end
  end
end

paths_by_linestrings = []
paths_by_link_id = []
0.upto(rendering_mode_list.length - 1) do |i|
  paths_by_linestrings[i] = []
  paths_by_link_id[i] = []
end

paths_by_points = []
length, time = 0, 0

puts "number of modes for rendering #{rendering_mode_list.length}"
rendering_mode_cursor = 0
0.upto(mode_count - 1) do |i|
  puts "creating path in line segment strings for mode #{modes[i]}"
  puts "rendering_mode_cursor: #{rendering_mode_cursor}"
  case modes[i]
  when 1001, 1002
    puts "Find features in street_lines..."
    0.upto(paths_by_vertices[i].length - 2) do |j|
      street_seg = StreetLines.find(:first, :conditions => [
          "(fnodeid = :from_id AND tnodeid = :to_id) OR (fnodeid = :to_id AND tnodeid = :from_id)",
          {:from_id => paths_by_vertices[i][j] % 100000000,
            :to_id => paths_by_vertices[i][j + 1] % 100000000}
        ])
      paths_by_linestrings[rendering_mode_cursor][j] = street_seg.the_geom
      edge = Edge.find_by_from_id_and_to_id_and_mode_id(paths_by_vertices[i][j],
        paths_by_vertices[i][j + 1], modes[i])
      length += edge.length
      time += edge.length * edge.speed_factor
      paths_by_link_id[rendering_mode_cursor][j] = edge.edge_id / 100
    end
    rendering_mode_cursor += 1
    puts "rendering_mode_cursor: #{rendering_mode_cursor}"
  when 1900
    puts "Find features in public transit layers..."
    0.upto(paths_by_vertices[i].length - 2) do |j|
      from_mode_id = Vertex.find_by_vertex_id(paths_by_vertices[i][j]).mode_id
      to_mode_id = Vertex.find_by_vertex_id(paths_by_vertices[i][j + 1]).mode_id
      if  from_mode_id != to_mode_id then
        # mode change, there must be a inner-PT switch point here
        switch_point = SwitchPoint.find_by_from_vertex_id_and_to_vertex_id(paths_by_vertices[i][j],
          paths_by_vertices[i][j + 1])
        rendering_mode_cursor += 1
        puts "rendering_mode_cursor: #{rendering_mode_cursor}"
      else
        case from_mode_id
        when 1002
          public_seg = StreetLines.find(:first, :conditions => [
              "(fnodeid = :from_id AND tnodeid = :to_id) OR (fnodeid = :to_id AND tnodeid = :from_id)",
              {:from_id => paths_by_vertices[i][j] % 100000000,
                :to_id => paths_by_vertices[i][j + 1] % 100000000}
            ])
          paths_by_linestrings[rendering_mode_cursor].push(public_seg.the_geom)
        when 1003
          # underground
          public_seg = UndergroundLines.find(:first, :conditions => [
              "(et_fnode = :from_id AND et_tnode = :to_id) OR (et_fnode = :to_id AND et_tnode = :from_id)",
              {:from_id => paths_by_vertices[i][j] % 100000000,
                :to_id => paths_by_vertices[i][j + 1] % 100000000}
            ])
          paths_by_linestrings[rendering_mode_cursor].push(public_seg.the_geom)
          puts "underground segment geom: #{paths_by_linestrings[rendering_mode_cursor][j]}"
        when 1004
          # suburban
          public_seg = SuburbanLines.find(:first, :conditions => [
              "(et_fnode = :from_id AND et_tnode = :to_id) OR (et_fnode = :to_id AND et_tnode = :from_id)",
              {:from_id => paths_by_vertices[i][j] % 100000000,
                :to_id => paths_by_vertices[i][j + 1] % 100000000}
            ])
          paths_by_linestrings[rendering_mode_cursor].push(public_seg.the_geom)
          puts "suburban segment geom: #{paths_by_linestrings[rendering_mode_cursor][j]}"
        when 1005
          # tram
          public_seg = TramLines.find(:first, :conditions => [
              "(et_fnode = :from_id AND et_tnode = :to_id) OR (et_fnode = :to_id AND et_tnode = :from_id)",
              {:from_id => paths_by_vertices[i][j] % 100000000,
                :to_id => paths_by_vertices[i][j + 1] % 100000000}
            ])
          paths_by_linestrings[rendering_mode_cursor].push(public_seg.the_geom)
          puts "tram segment geom: #{paths_by_linestrings[rendering_mode_cursor][j]}"
        end
        edge = Edge.find_by_from_id_and_to_id_and_mode_id(paths_by_vertices[i][j],
          paths_by_vertices[i][j + 1], from_mode_id)
        length += edge.length
        time += edge.length * edge.speed_factor
        paths_by_link_id[rendering_mode_cursor].push(edge.edge_id / 100)
      end
    end
  else
    puts "Find features in some other layers NOT supported yet..."
  end
end

0.upto(rendering_mode_list.length - 1) do |i|
  if (paths_by_linestrings[i].length == 1) then
    paths_by_points[i] = paths_by_linestrings[i][0].geometries[0].points
  else
    if (paths_by_linestrings[i][0].geometries[0][0] == paths_by_linestrings[i][1].geometries[0][0]) || ((paths_by_linestrings[i][0].geometries[0][0] == paths_by_linestrings[i][1].geometries[0][-1])) then
      paths_by_points[i] = paths_by_linestrings[i][0].geometries[0].points.reverse
    else
      paths_by_points[i] = paths_by_linestrings[i][0].geometries[0].points
    end
    1.upto(paths_by_linestrings[i].length - 1) do |j|
      if paths_by_linestrings[i][j].geometries[0][-1] == paths_by_points[i][-1] then
        paths_by_points[i] += paths_by_linestrings[i][j].geometries[0].points.reverse
      else
        paths_by_points[i] += paths_by_linestrings[i][j].geometries[0].points
      end
    end
  end
  #  puts paths_by_points[i]
end