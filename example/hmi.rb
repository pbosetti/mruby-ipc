#!mruby

get "/" do
  <<-EOH
  <!DOCTYPE html>
  <html>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
  <title>HMI test</title>
  <body>
  <h1>HMI test</h1>
  <a href="/step">Make a step</a>
  <a href="/ipc.txt">Inspect IPC</a>
  <a href="/echo?name=John&surname=Carter">Echo test</a>
  <a href="/stop">FULL STOP</a>  
  </body>
  </html>
  EOH
end

get "/step" do 
  bytes = $ipc.send_with_sep "ping"
  res = $ipc.receive_to_sep
  $ipc.message "res is #{res}"
  <<-EOH
  <meta http-equiv=refresh content='3; URL=/'>
  Sent #{bytes} bytes, got #{res}
  EOH
end

get "/stop" do 
  $ipc.send_with_sep "stop"
  res = $ipc.receive_to_sep
  #UV.default_loop.stop
  "Sent stop signal, got #{res}"
end


get "/ipc.txt" do 
  "#{$ipc.inspect}"
end

get "/options.yaml" do |r, param|
  YAML.dump(Sinatic.options)
end

get "/echo" do |r, param|
  YAML.dump({
    r:r,
    query:r.query,
    pairs:query(r), #Gets query string as a hash
    param:Sinatic.options
    })
end

