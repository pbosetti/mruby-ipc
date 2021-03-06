
# ipc = IPC.new()
# ipc.message "Forking..."
# ipc.full_fork

IPC.fork do |ipc|

if ipc.child? then # CHILD PROCESS
  ipc.message "I am the #{ipc.role}"
  ipc.message "ipc: #{ipc.inspect}"

  10.times do |i|
    n = ipc.send_with_sep "prova #{i}"
    ipc.message "[#{i}] wrote #{n} bytes to pipe #{ipc.writepipe}"
    sleep 0.05
  end
  ipc.send_with_sep "stop"
  y = {"one" => 1, "two" => 2, "d" =>%W(Paolo Bosetti), :c => "test"}
  ipc.message y.inspect
  ipc.send_with_sep YAML.dump(y), "$$"


else # PARENT PROCESS
  ipc.message "I am the #{ipc.role}"
  ipc.message "ipc: #{ipc.inspect}"

  sleep 0.1
  ipc.message "reading from #{ipc.readpipe}"
  i = 0
  while (true)
    begin
      msg = ipc.receive_to_sep
    rescue IPCPipeError
      ipc.message "pipe closed"
      break
    end
    if msg then
      ipc.message "Got: '#{msg}'"
      break if msg == "stop"
    end
  end
  y = ipc.receive_to_sep "$$"
  ipc.message y.inspect
  ipc.message YAML.load(y).inspect
end

ipc.message "#{ipc.role} exiting"
end