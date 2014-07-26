# To change this template, choose Tools | Templates
# and open the template in the editor.

module Mmspa
  class SwitchCondition
    attr_accessor :type, :cost, :is_available
    
    def initialize
      @type = ''
      @cost = 0.0
      @is_available = true
    end
  end
end
