#module Mmspa
#	extend FFI::Library
#  ffi_lib "/usr/lib/libmmspa4pg.so.0.2"
#  attach_function :ConnectDB, [:string], :int
#  attach_function :DisconnectDB, [], :void
#	attach_function :Parse, [], :int
#	attach_function :CreateRoutingPlan, [:int, :int], :void
#	attach_function :SetModeListItem, [:int, :int], :void
#  attach_function :SetPublicTransitModeSetItem, [:int, :int], :void
#	attach_function :SetSwitchConditionListItem, [:int, :string], :void
#  callback :VertexValidationChecker, [:pointer], :int
#  attach_function :SetSwitchingConstraint, [:int, :VertexValidationChecker], :void
#  attach_function :SetTargetConstraint, [:VertexValidationChecker], :void
#	attach_function :SetCostFactor, [:string], :void
#	attach_function :MultimodalTwoQ, [:long_long], :void
#	attach_function :GetFinalPath, [:long_long, :long_long], :pointer
#  attach_function :GetFinalCost, [:long_long, :string], :double
#	attach_function :Dispose, [], :void
#  attach_function :DisposePaths, [:pointer], :void
#
#  class Path < FFI::Struct
#    layout  :vertex_list,         :pointer,
#      :vertex_list_length,  :int
#  end
#
#  class MemoryVertex < FFI::Struct
#    layout  :id,                  :long_long,
#      :temp_cost,           :double,
#      :distance,            :double,
#      :elapsed_time,        :double,
#      :walking_distance,    :double,
#      :walking_time,        :double,
#      :parent,              :pointer,
#      :outdegree,           :int,
#      :outgoing,            :pointer,
#      :status,              :int,
#      :next,                :pointer
#  end
#end
