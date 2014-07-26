# To change this template, choose Tools | Templates
# and open the template in the editor.

module Mmspa
  class RoutingResult
    attr_accessor :is_existent, :planned_mode_list, :rendering_mode_list, :switch_point_list,
      :switch_type_list, :switch_point_name_list, :planned_switch_type_list, :paths_by_vertex_id, :paths_by_edge_id,
      :paths_by_link_id, :paths_by_points, :description, :length, :time, :walking_length, :walking_time

    def initialize
      @is_existent = false
      @planned_mode_list = []
      @rendering_mode_list = []
      @paths_by_vertex_id = []
      @paths_by_edge_id = []
      @paths_by_link_id = []
      @paths_by_points = []
      @switch_type_list = []
      @planned_switch_type_list = []
      @switch_point_list = []
      @switch_point_name_list = []
      @description = ''
      @length = 0.0
      @time = 0.0
      @walking_length = 0.0
      @walking_time = 0.0
    end
  end
end
