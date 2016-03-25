
ipc = IPC.new()
# ipc.bufsize = 5
ipc.message "Forking..."
ipc.full_fork

if ipc.child? then # CHILD PROCESS
  ipc.message "I am the #{ipc.role}"
  ipc.message "ipc: #{ipc.inspect}"

  1000.times do |i|
    n = ipc.send "prova #{i} "
    ipc.message "[#{i}] wrote #{n} bytes to pipe #{ipc.writepipe}"
    sleep 0.05
  end
  ipc.send "stop"
  ipc.message "#{ipc.role} exiting"


else # PARENT PROCESS
  ipc.message "I am the #{ipc.role}"
  ipc.message "ipc: #{ipc.inspect}"
  
  sleep 0.5
  ipc.message "reading from #{ipc.readpipe}"
  i = 0
  while (true)
    begin
      msg = ipc.receive
    rescue IPCPipeError
      ipc.message "pipe closed"
      break
    end
    ipc.message "Got: '#{msg}'" if msg
    break if msg == "stop"
  end
  sleep 1
  ipc.message "#{ipc.role} exiting"
end
