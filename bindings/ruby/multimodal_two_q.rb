# To change this template, choose Tools | Templates
# and open the template in the editor.

module Mmspa
  class MultimodalTwoQ < MultimodalRoutePlanner
    attr_reader :final_path, :final_cost
    attr_accessor :dists, :preds

    def initialize(routing_plan)
      @final_path = Array.new
      @final_cost = 0.0
      @routing_plan = routing_plan
      @dists = Array.new
      @preds = Array.new
      @front_q = Array.new
      @back_q = Array.new
      @node_state_dict = Hash.new
    end
    
    def search_path?(source, target)
      start_time = Time.now
      @source = source
      @target = target
      if !input_valid? then return false
      end

      0.upto(@routing_plan.mode_list.count - 1) do |i|
        @dists[i] = Hash.new
        @preds[i] = Hash.new
        multimodal_init(i)
        two_q(i)
      end

      construct_path
      end_time = Time.now
      ellapsed = end_time - start_time
      puts "time consumed: #{ellapsed.to_s}"
    end
    
    private
    
    def input_valid?
      puts "source = #{@source}"
      puts "target = #{@target}"
      first_mode_id = Mode.find_by_mode_name(@routing_plan.mode_list[0]).mode_id
      last_mode_id = Mode.find_by_mode_name(@routing_plan.mode_list[-1]).mode_id
      puts "#{first_mode_id}"
      puts "#{last_mode_id}"
      source_vertex = Vertex.find_by_mode_id_and_vertex_id(first_mode_id, @source)
      target_vertex = Vertex.find_by_mode_id_and_vertex_id(last_mode_id, @target)
      if source_vertex && target_vertex
        if source_vertex.out_degree > 0 && Edge.find_by_mode_id_and_to_id(last_mode_id, @target)
          return true
        end
      end
      return false
    end
    
    def multimodal_init(mode_index)
      mode_name = @routing_plan.mode_list[mode_index]
      puts "doing multimodal_two_q init on #{mode_name} graph..."
      @node_state_dict = Hash.new
      mode_id = Mode.find_by_mode_name(mode_name).mode_id
      Vertex.find_all_by_mode_id(mode_id).each do |v|
        @dists[mode_index][v.vertex_id.to_s] = Float::MAX
        @preds[mode_index][v.vertex_id.to_s] = 'NULL'
        @node_state_dict[v.vertex_id.to_s] = 'UNREACHED'
      end
      if mode_index == 0
        @dists[mode_index][@source.to_s] = 0.0
        @preds[mode_index][@source.to_s] = @source.to_s
        @back_q.push(@source.to_s)
        @node_state_dict[@source.to_s] = 'IN_QUEUE'
      else
        switch_condition = @routing_plan.switch_condition_list[mode_index - 1]
        type_id = SwitchType.find_by_type_name(switch_condition.type).type_id
        SwitchPoint.find_all_by_type_id_and_cost_and_is_available(type_id,
          switch_condition.cost, switch_condition.is_available).each do |sp|
          @dists[mode_index][sp.to_vertex_id.to_s] = @dists[mode_index - 1][sp.from_vertex_id.to_s]
          @preds[mode_index][sp.to_vertex_id.to_s] = sp.to_vertex_id.to_s
          @back_q.push(sp.to_vertex_id.to_s)
          @node_state_dict[sp.to_vertex_id.to_s] = 'IN_QUEUE'
        end
      end
      puts "multimodal_two_q init on #{mode_name} graph complete"
    end
    
    def two_q(mode_index)
      mode_name = @routing_plan.mode_list[mode_index]
      puts "doing two_q on #{mode_name} graph..."
      mode_id = Mode.find_by_mode_name(mode_name).mode_id
      while !(@front_q.empty?) || !(@back_q.empty?)
        node_from = @front_q.empty? ? @back_q.shift : @front_q.shift
        #puts node_from
        @node_state_dict[node_from] = 'WAS_IN_QUEUE'
        #puts mode_id
        vertex_from = Vertex.find_by_vertex_id_and_mode_id(node_from.to_f, mode_id)
        #puts vertex_from
        if vertex_from.out_degree > 0
          first_edge_record_id = Edge.find_by_edge_id_and_mode_id(vertex_from.first_edge, mode_id).id
          vertex_from.out_degree.times do |i|
            #puts i.to_s
            edge = Edge.find(first_edge_record_id + i)
            edge_cost = if @routing_plan.optimizing_field == 'time'
                          edge.length * edge.speed_factor
                        elsif @routing_plan.optimizing_field == 'length'
                          edge.length
                        end
            #puts edge_cost.to_s
            new_cost = @dists[mode_index][node_from] + edge_cost
            #puts new_cost.to_s
            node_to = edge.to_id.to_s
            if new_cost < @dists[mode_index][node_to]
              @dists[mode_index][node_to] = new_cost
              @preds[mode_index][node_to] = node_from
              if !(@front_q.include?(node_to)) && !(@back_q.include?(node_to))
                if @node_state_dict[node_to] == 'WAS_IN_QUEUE'
                  @front_q.push(node_to)
                else
                  @back_q.push(node_to)
                end
                @node_state_dict[node_to] = 'IN_QUEUE'
              end
            end
          end
        end
      end
      puts "two_q on #{mode_name} graph complete"
    end
    
    def construct_path
      @final_cost = @dists[@routing_plan.mode_list.count - 1][@target.to_s]
      puts "final cost is #{@final_cost.to_s}"
      puts "construct final path here"
    end

  end
end
