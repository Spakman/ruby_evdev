while(line = gets)
  if(/define\s+([A-Za-z]+)_(\w+)\s+(\S+)/ =~ line)
    puts "@#{$1}[#{$3}] = '#{$2}'"
  end
end
