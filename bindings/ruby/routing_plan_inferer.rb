# To change this template, choose Tools | Templates
# and open the template in the editor.

module InferenceEngine
  class RoutingPlanInferer
    def initialize
      
    end

    def self.generate_routing_plan(options)
      routing_plan_list = []
      cost_factor = case options.objective
      when 'shortest' then 'length'
      when 'fastest' then 'speed'
      end
      if options.objective == 'fastest'
        if !options.can_use_public
          if !options.has_private_car
            # only can walk, mono-modal routing
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('foot').mode_id
            routing_plan.is_multimodal = false
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Walking'
            routing_plan_list[0] = routing_plan
            return routing_plan_list
          end

          if options.has_private_car and !options.need_parking
            # somebody else will be the driver
            # the car can be parked temporarily anywhere
            # There are 3 possible mode combinations in this case:
            # car; foot; car-foot with geo_connection as Switch Point
            # 1st: car only
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('private_car').mode_id
            routing_plan.is_multimodal = false
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Driving a car'
            routing_plan_list[0] = routing_plan
            # 2nd: foot only
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('foot').mode_id
            routing_plan.is_multimodal = false
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Walking'
            routing_plan_list[1] = routing_plan
            # 3rd: car-foot with geo_connection as Switch Point
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('private_car').mode_id
            routing_plan.mode_list[1] = Mode.find_by_mode_name('foot').mode_id
            type_id = SwitchType.find_by_type_name('geo_connection').type_id
            routing_plan.switch_type_list[0] = type_id
            routing_plan.switch_condition_list[0] = "type_id=" + type_id.to_s + " AND is_available=true"
            routing_plan.switch_constraint_list[0] = nil
            routing_plan.is_multimodal = true
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'By car first, then walking without parking'
            routing_plan_list[2] = routing_plan
            return routing_plan_list
          end

          if options.has_private_car and options.need_parking
            # the user may be the driver
            # and he/she surely need a parking lot for the car
            # There are also 2 possible mode combinations in this case:
            # foot; car-foot with parking as Switch Point            
            # 1st: foot only
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('foot').mode_id
            routing_plan.is_multimodal = false
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Walking'
            routing_plan_list[0] = routing_plan
            # 2nd: car-foot with parking lots as Switch Point
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('private_car').mode_id
            routing_plan.mode_list[1] = Mode.find_by_mode_name('foot').mode_id
            type_id = SwitchType.find_by_type_name('car_parking').type_id
            routing_plan.switch_type_list[0] = type_id
            routing_plan.switch_condition_list[0] = "type_id=" + type_id.to_s + " AND is_available=true"
            routing_plan.switch_constraint_list[0] = nil
            routing_plan.is_multimodal = true
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Driving, parking and walking'
            routing_plan_list[1] = routing_plan
            return routing_plan_list
          end

        else
          # can use public transportation system
          # 2010-02-01 important comments by Liu Lu
          # !!!IMPORTANT!!!: according to the latest idea in my head, in this
          #                  "can use public transportation" branch, the selected
          #                  PT modes networks should be combined together with
          #                  the pedestrian network. This process can avoid the
          #                  redundant inferred routing plans e.g. foot-PT-foot,
          #                  PT-foot, foot-PT etc.
          #                  As a result, the new PT(1900) mode here means the
          #                  network composed of pedestrian and selected PT modes
          #                  networks
          #
          if !options.has_private_car
            # the user can walk or take public transportation
            # 2 possible mode combinations:
            # 1. foot;
            # 2. PT
            #
            # 1: foot only
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('foot').mode_id
            routing_plan.is_multimodal = false
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Walking'
            routing_plan_list[0] = routing_plan
            # 2: public transportation
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.has_public_transit = true
            routing_plan.mode_list[0] = Mode.find_by_mode_name('public_transportation').mode_id
            0.upto(options.available_public_modes.length - 1) do |i|
              routing_plan.public_transit_set[i] = Mode.find_by_mode_name(options.available_public_modes[i]).mode_id
            end
            routing_plan.is_multimodal = true
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Walking and taking public transit'
            routing_plan_list[1] = routing_plan
            return routing_plan_list
          end

          if options.has_private_car and !options.need_parking
            # somebody else will be the driver
            # the car can be parked temporarily anywhere
            # There are 5 possible mode combinations in this case:
            # 1. car;
            # 2. foot;
            # 3. car-foot with geo_connection as Switch Point;
            # 4. PT;
            # 5. car-PT with geo_connection as Switch Point;
            # 6. car-PT with kiss+R as Switch Point;
            #
            # 1: car only
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('private_car').mode_id
            routing_plan.is_multimodal = false
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Driving a car'
            routing_plan_list[0] = routing_plan
            # 2: foot only
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('foot').mode_id
            routing_plan.is_multimodal = false
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Walking'
            routing_plan_list[1] = routing_plan
            # 3: car-foot with geo_connection as Switch Point
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('private_car').mode_id
            routing_plan.mode_list[1] = Mode.find_by_mode_name('foot').mode_id
            type_id = SwitchType.find_by_type_name('geo_connection').type_id
            routing_plan.switch_type_list[0] = type_id
            routing_plan.switch_condition_list[0] = "type_id=" + type_id.to_s + " AND is_available=true"
            routing_plan.switch_constraint_list[0] = nil
            routing_plan.is_multimodal = true
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'By car first, then walking without parking'
            routing_plan_list[2] = routing_plan
            # 4: public transportation
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.has_public_transit = true
            routing_plan.mode_list[0] = Mode.find_by_mode_name('public_transportation').mode_id
            0.upto(options.available_public_modes.length - 1) do |i|
              routing_plan.public_transit_set[i] = Mode.find_by_mode_name(options.available_public_modes[i]).mode_id
            end
            routing_plan.is_multimodal = true
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Walking and taking public transit'
            routing_plan_list[3] = routing_plan
            # 5: car-PT with geo_connection as Switch Point
#            routing_plan = Mmspa::RoutingPlan.new
#            routing_plan.has_public_transit = true
#            routing_plan.mode_list[0] = Mode.find_by_mode_name('private_car').mode_id
#            routing_plan.mode_list[1] = Mode.find_by_mode_name('public_transportation').mode_id
#            type_id = SwitchType.find_by_type_name('geo_connection').type_id
#            routing_plan.switch_type_list[0] = type_id
#            routing_plan.switch_condition_list[0] = "type_id=" + type_id.to_s + " AND is_available=true"
#            routing_plan.switch_constraint_list[0] = nil
#            0.upto(options.available_public_modes.length - 1) do |i|
#              routing_plan.public_transit_set[i] = Mode.find_by_mode_name(options.available_public_modes[i]).mode_id
#            end
#            routing_plan.is_multimodal = true
#            routing_plan.cost_factor = cost_factor
#            routing_plan.description = 'Driving and taking public transit'
#            routing_plan_list[4] = routing_plan
            # 6: car-PT with kiss+R as Switch Point
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.has_public_transit = true
            routing_plan.mode_list[0] = Mode.find_by_mode_name('private_car').mode_id
            routing_plan.mode_list[1] = Mode.find_by_mode_name('public_transportation').mode_id
            type_id = SwitchType.find_by_type_name('kiss_and_ride').type_id
            routing_plan.switch_type_list[0] = type_id
            routing_plan.switch_condition_list[0] = "type_id=" + type_id.to_s + " AND is_available=true"
            routing_plan.switch_constraint_list[0] = nil
            0.upto(options.available_public_modes.length - 1) do |i|
              routing_plan.public_transit_set[i] = Mode.find_by_mode_name(options.available_public_modes[i]).mode_id
            end
            routing_plan.is_multimodal = true
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Driving and taking public transit via Kiss+R'
            routing_plan_list[4] = routing_plan
            return routing_plan_list
          end

          if options.has_private_car and options.need_parking
            # the user may be the driver
            # and he/she surely need a parking lot for the car
            # There are 6 possible mode combinations in this case:
            # 1. foot;
            # 2. car-foot with parking as Switch Point;
            # 3. PT;
            # 4. car-PT with parking as Switch Points;
            # 5. car-PT with P+R as Switch Points;
            #
            # 1: foot only
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('foot').mode_id
            routing_plan.is_multimodal = false
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Walking'
            routing_plan_list[0] = routing_plan
            # 2: car-foot with parking lots as Switch Point
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('private_car').mode_id
            routing_plan.mode_list[1] = Mode.find_by_mode_name('foot').mode_id
            type_id = SwitchType.find_by_type_name('car_parking').type_id
            routing_plan.switch_type_list[0] = type_id
            routing_plan.switch_condition_list[0] = "type_id=" + type_id.to_s + " AND is_available=true"
            routing_plan.switch_constraint_list[0] = nil
            routing_plan.is_multimodal = true
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Driving, parking and walking'
            routing_plan_list[1] = routing_plan
            # 3: public transportation
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.has_public_transit = true
            routing_plan.mode_list[0] = Mode.find_by_mode_name('public_transportation').mode_id
            0.upto(options.available_public_modes.length - 1) do |i|
              routing_plan.public_transit_set[i] = Mode.find_by_mode_name(options.available_public_modes[i]).mode_id
            end
            routing_plan.is_multimodal = true
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Walking and taking public transit'
            routing_plan_list[2] = routing_plan
            # 4: car-PT with parking as Switch Point
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.has_public_transit = true
            routing_plan.mode_list[0] = Mode.find_by_mode_name('private_car').mode_id
            routing_plan.mode_list[1] = Mode.find_by_mode_name('public_transportation').mode_id
            type_id = SwitchType.find_by_type_name('car_parking').type_id
            routing_plan.switch_type_list[0] = type_id
            routing_plan.switch_condition_list[0] = "type_id=" + type_id.to_s + " AND is_available=true"
            routing_plan.switch_constraint_list[0] = nil
            0.upto(options.available_public_modes.length - 1) do |i|
              routing_plan.public_transit_set[i] = Mode.find_by_mode_name(options.available_public_modes[i]).mode_id
            end
            routing_plan.is_multimodal = true
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Driving and taking public transit'
            routing_plan_list[3] = routing_plan
            # 5: car-PT with P+R as Switch Point
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.has_public_transit = true
            routing_plan.mode_list[0] = Mode.find_by_mode_name('private_car').mode_id
            routing_plan.mode_list[1] = Mode.find_by_mode_name('public_transportation').mode_id
            type_id = SwitchType.find_by_type_name('park_and_ride').type_id
            routing_plan.switch_type_list[0] = type_id
            routing_plan.switch_condition_list[0] = "type_id=" + type_id.to_s + " AND is_available=true"
            routing_plan.switch_constraint_list[0] = nil
            0.upto(options.available_public_modes.length - 1) do |i|
              routing_plan.public_transit_set[i] = Mode.find_by_mode_name(options.available_public_modes[i]).mode_id
            end
            routing_plan.is_multimodal = true
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Driving and taking public transit via P+R'
            routing_plan_list[4] = routing_plan
            return routing_plan_list
          end
        end

      elsif options.objective == 'shortest'
        if !options.can_use_public
          if !options.has_private_car
            # no car, walk only
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('foot').mode_id
            routing_plan.is_multimodal = false
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Walking'
            routing_plan_list[0] = routing_plan
            return routing_plan_list
          else
            # car
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('private_car').mode_id
            routing_plan.is_multimodal = false
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Driving a car'
            routing_plan_list[0] = routing_plan
            # foot
            routing_plan = Mmspa::RoutingPlan.new
            routing_plan.mode_list[0] = Mode.find_by_mode_name('foot').mode_id
            routing_plan.is_multimodal = false
            routing_plan.cost_factor = cost_factor
            routing_plan.description = 'Walking'
            routing_plan_list[1] = routing_plan
            return routing_plan_list
          end
        else

        end
      end
    end
  end
end
