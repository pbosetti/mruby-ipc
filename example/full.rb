$ipc = IPC.fork

if $ipc.child? then # CHILD PROCESS
  require "./example/hmi.rb"
  Sinatic.run :host => '0.0.0.0', :port => 8080

else # PARENT PROCESS
  require "./example/fsm.rb"
  
end

$ipc.message "#{$ipc.role} exiting"
