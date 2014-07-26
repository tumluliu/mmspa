# To change this template, choose Tools | Templates
# and open the template in the editor.

module Mmspa
  class RoutingPlan
    attr_accessor :is_multimodal, :mode_list, :switch_condition_list, :switch_type_list,
      :switch_constraint_list, :target_constraint, :public_transit_set,
      :has_public_transit, :cost_factor, :description

    def initialize
      @is_multimodal = false
      @mode_list = []
      @switch_type_list = []
      @switch_condition_list = []
      @switch_constraint_list = []
      @target_constraint = nil
      @public_transit_set = []
      @has_public_transit = false
      @cost_factor = ''
      @description = ''
    end
  end
end
