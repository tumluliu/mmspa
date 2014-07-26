# To change this template, choose Tools | Templates
# and open the template in the editor.

module InferenceEngine
  class RoutingOptions
    attr_accessor :can_use_public, :can_use_taxi, :has_bicycle, :has_motorcycle,
      :has_private_car, :need_parking, :objective, :available_public_modes
    attr_reader :can_use_underground, :can_use_suburban, :can_use_tram,
      :can_use_bus

    def initialize
      @can_use_public = @can_use_underground = @can_use_suburban =
        @can_use_tram = @can_use_bus = @can_use_taxi = @has_bicycle =
        @has_motorcycle = @has_private_car = @need_parking = false
      @objective = ''
      @available_public_modes = Array.new
    end

    def can_use_underground(can_use)
      @can_use_underground = can_use
      @available_public_modes.push("underground")
    end

    def can_use_suburban(can_use)
      @can_use_suburban = can_use
      @available_public_modes.push("suburban")
    end

    def can_use_tram(can_use)
      @can_use_tram = can_use
      @available_public_modes.push("tram")
    end

    def can_use_bus(can_use)
      @can_use_bus = can_use
      @available_public_modes.push("bus")
    end

    def generate_id
      id = []
      if @can_use_public then id[0] = "1" else id[0] = "0" end
      if @can_use_underground then id[1] = "1" else id[1] = "0" end
      if @can_use_suburban then id[2] = "1" else id[2] = "0" end
      if @can_use_tram then id[3] = "1" else id[3] = "0" end
      if @can_use_bus then id[4] = "1" else id[4] = "0" end
      if @can_use_taxi then id[5] = "1" else id[5] = "0" end
      if @has_bicycle then id[6] = "1" else id[6] = "0" end
      if @has_motorcycle then id[7] = "1" else id[7] = "0" end
      if @has_private_car then id[8] = "1" else id[8] = "0" end
      if @need_parking then id[9] = "1" else id[9] = "0" end
      if @objective == 'shortest' then
        id[10] = "0"
      elsif @objective == 'fastest' then
        id[10] = "1"
      else
        id[10] = "2"
      end
      id.to_s
    end
  end
end
