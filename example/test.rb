
a = 10
ipc = IPC.new()
# ipc.bufsize = 5
ipc.message "Forking..."
ipc.full_fork
n = 0
if ipc.child? then
  ipc.message "I am the #{ipc.role}, a=#{a}"
  ipc.message "ipc: #{ipc.inspect}"
  ipc.message "descriptors: #{ipc.descriptors}"

  10.times do |i|
    n = ipc.send "prova #{i}"
    ipc.message "[#{i}] wrote #{n} bytes to pipe #{ipc.writepipe}"
    sleep 0.25
  end
  ipc.send "stop"
  ipc.message "#{ipc.role} exiting"
else
  a += 1
  ipc.message "I am the #{ipc.role}, a=#{a}"
  ipc.message "ipc: #{ipc.inspect}"
  ipc.message "descriptors: #{ipc.descriptors}"
  
  sleep 0.5
  ipc.message "reading from #{ipc.readpipe}"
  i = 0
  while (true)
    msg = ipc.receive
    ipc.message "Got: '#{msg}'" if msg
    break if msg == "stop"
  end
  # while (true) do
  #   msg = ipc.receive
  #   break if (msg.length > 0) or (i > 10)
  #   i += 1
  #   sleep 0.1
  # end
  # ipc.message "Got: '#{msg}'"
  sleep 1
  ipc.message "#{ipc.role} exiting"
end
