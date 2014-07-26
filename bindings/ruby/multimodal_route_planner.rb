# To change this template, choose Tools | Templates
# and open the template in the editor.

module Mmspa
  class MultimodalRoutePlanner
    def initialize

    end

    def self.input_valid?(first_mode_id, last_mode_id, source, target, public_modes)
      RAILS_DEFAULT_LOGGER.info "checking input validation..."
      RAILS_DEFAULT_LOGGER.info "source = #{source}"
      RAILS_DEFAULT_LOGGER.info "target = #{target}"
      source_vertex = target_vertex = nil
      if first_mode_id != 1900 and last_mode_id != 1900 then
        source_vertex = Vertex.find_by_mode_id_and_vertex_id(first_mode_id, source)
        target_vertex = Vertex.find_by_mode_id_and_vertex_id(last_mode_id, target)
      elsif first_mode_id == 1900 and last_mode_id != 1900 then
        target_vertex = Vertex.find_by_mode_id_and_vertex_id(last_mode_id, target)
        source_vertex = Vertex.find_by_mode_id_and_vertex_id(1002, source)
        if source_vertex == nil then
          0.upto(public_modes.length - 1) do |i|
            source_vertex = Vertex.find_by_mode_id_and_vertex_id(public_modes[i], source)
            if source_vertex != nil then
              break
            end
          end
        end
      elsif first_mode_id != 1900 and last_mode_id == 1900 then
        source_vertex = Vertex.find_by_mode_id_and_vertex_id(first_mode_id, source)
        target_vertex = Vertex.find_by_mode_id_and_vertex_id(1002, target)
        if target_vertex == nil then
          0.upto(public_modes.length - 1) do |i|
            target_vertex = Vertex.find_by_mode_id_and_vertex_id(public_modes[i], target)
            if target_vertex != nil then
              break
            end
          end
        end
      elsif first_mode_id == 1900 and last_mode_id == 1900 then
        source_vertex = Vertex.find_by_mode_id_and_vertex_id(1002, source)
        if source_vertex == nil then
          0.upto(public_modes.length - 1) do |i|
            source_vertex = Vertex.find_by_mode_id_and_vertex_id(public_modes[i], source)
            if source_vertex != nil then
              break
            end
          end
        end
        target_vertex = Vertex.find_by_mode_id_and_vertex_id(1002, target)
        if target_vertex == nil then
          0.upto(public_modes.length - 1) do |i|
            target_vertex = Vertex.find_by_mode_id_and_vertex_id(public_modes[i], target)
            if target_vertex != nil then
              break
            end
          end
        end
      end

      if (source_vertex != nil) && (target_vertex != nil) then
        if (source_vertex.out_degree > 0) && (Edge.find_all_by_mode_id_and_to_id(target_vertex.mode_id, target).count > 0) then
          RAILS_DEFAULT_LOGGER.info "input valid!"
          return true
        end
      end
      RAILS_DEFAULT_LOGGER.info "input invalid!"
      return false
    end
  end
end
