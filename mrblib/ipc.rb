#*************************************************************************#
#                                                                         #
# raspberry.rb - mruby gem provoding access to Raspberry Pi IOs           #
# Copyright (C) 2015 Paolo Bosetti                                        #
# paolo[dot]bosetti[at]unitn.it                                           #
# Department of Industrial Engineering, University of Trento              #
#                                                                         #
# This library is free software.  You can redistribute it and/or          #
# modify it under the terms of the GNU GENERAL PUBLIC LICENSE 2.0.        #
#                                                                         #
# This library is distributed in the hope that it will be useful,         #
# but WITHOUT ANY WARRANTY; without even the implied warranty of          #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           #
# Artistic License 2.0 for more details.                                  #
#                                                                         #
# See the file LICENSE                                                    #
#                                                                         #
#*************************************************************************#

class IPCError; end
class IPCPipeError < IPCError; end

class IPC
  attr_reader :role, :forked, :bufsize
  
  def bufsize=(v)
    raise ArgumentError unless v.kind_of? Numeric
    raise ArgumentError unless (0..65536).include? v.to_i 
    @bufsize = v
  end
  
  def full_fork
    if (pid = self.fork) == 0 then
      self.as_child
    else
      self.as_parent
    end
    return pid
  end
  
  def parent?
    return @role == "parent"
  end
  
  def child?
    return @role == "child"
  end
  
  def inspect
    s = self.to_s[0..-2]
    s += " @pid=#{self.pid} @role=#{self.role} @writepipe=#{self.writepipe}, @readpipe=#{self.readpipe} @bufsize=#{@bufsize}>"
  end
  
  def message(s)
    if @role == "parent"
      puts s.green
    elsif @role == "child"
      puts s.red      
    else
      puts s.yellow
    end
  end
end
