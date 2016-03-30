#!mruby

get "/" do
  "Hello, World!"
end

get "/step" do 
  bytes = $ipc.send_with_sep "ping"
  res = $ipc.receive_to_sep
  "Sent #{bytes} bytes, got #{res}"
end

get "/stop" do 
  $ipc.send_with_sep "stop"
  res = $ipc.receive_to_sep
  "Sent stop signal, got #{res}"
end


get "/ipc.txt" do 
  "#{$ipc.inspect}"
end

get "/options.yaml" do |r, param|
  YAML.dump(Sinatic.options)
end

get "/echo.yaml" do |r, param|
  YAML.dump({
    r:r,
    query:r.query,
    pairs:query(r), #Gets query string as a hash
    param:param
    })
end

