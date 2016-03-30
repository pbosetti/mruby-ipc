m = FSM::Machine.new(:count)
# set values for parameters:
m.params.count = 0

# STEP 2
# Create states, giving a name (String)
idle_state = FSM::State.new "Idle"
# define actions for on_enter, in_loop, and on_exit (using a DSL):
idle_state.on_enter { $ipc.message "> Entering #{self.name}"}
idle_state.in_loop do

  begin
    msg = $ipc.receive_to_sep
  rescue IPCPipeError
    $ipc.message "pipe closed"
    transition_to 'Stop'
  end
  
  if msg then
    $ipc.message "Got: '#{msg}'"
    params.count += 1
    $ipc.send_with_sep params.count.to_s
    if params.count > 10 or msg == "stop"
      transition_to 'Stop'
    end
  end

end
idle_state.on_exit { $ipc.message "< Exiting #{self.name}"}
# If needed, define a timer for the state (in seconds):
idle_state.timing = 0.05
# finally add the state to the FSM instance:
m.add idle_state

# Repeat for other states:
stop_state = FSM::State.new "Stop"
stop_state.on_enter { $ipc.message "> Entering #{self.name}"}
stop_state.in_loop { stop_machine }
stop_state.on_exit { $ipc.message "< Exiting #{self.name}"}
m.add stop_state

# STEP 3
# set $debug to true for enabling warn(message)
# $debug = true
# run the FSM:
m.run "Idle"