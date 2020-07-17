class BindingGen
  def initialize
    @bindings = {}
  end

  def binding(*args, **kwargs)
    b = Binding.new(*args, **kwargs)
    b.increment_name while @bindings[b.name]
    @bindings[b.name] = b
    b.write
  end

  def init
    puts 'void init_bindings(Env *env) {'
    @consts = {}
    @bindings.values.each do |binding|
      unless @consts[binding.rb_class]
        puts "    Value *#{binding.rb_class} = NAT_OBJECT->const_get(env, #{binding.rb_class.inspect}, true);"
        @consts[binding.rb_class] = true
      end
      puts "    #{binding.rb_class}->define_method(env, #{binding.rb_method.inspect}, #{binding.name});"
    end
    puts '}'
  end

  class Binding
    def initialize(rb_class, rb_method, cpp_class, cpp_method, argc:, pass_env:, pass_block:, return_type:)
      @rb_class = rb_class
      @rb_method = rb_method
      @cpp_class = cpp_class
      @cpp_method = cpp_method
      @argc = argc
      @pass_env = pass_env
      @pass_block = pass_block
      @return_type = return_type
      generate_name
    end

    attr_reader :rb_class, :rb_method, :cpp_class, :cpp_method, :argc, :pass_env, :pass_block, :return_type, :name

    def write
      puts <<-FUNC
Value *#{name}(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    #{argc_assertion}
    #{cpp_class} *self = self_value->#{as_method_name}();
    auto return_value = self->#{cpp_method}(#{env_arg} #{args} #{block_arg});
    #{return_code}
}\n
      FUNC
    end

    def increment_name
      @name = @name.sub(/_binding(\d*)$/) { "_binding#{$1.to_i + 1}" }
    end

    private

    def argc_assertion
      if Range === argc
        "NAT_ASSERT_ARGC(#{argc.begin}, #{argc.end});"
      else
        "NAT_ASSERT_ARGC(#{argc});"
      end
    end

    def env_arg
      "#{pass_env ? 'env' : ''}#{pass_env && max_argc > 0 ? ',' : ''}"
    end

    def args
      (0...max_argc).map do |i|
        "argc >= #{max_argc} ? args[#{i}] : nullptr"
      end.join(', ')
    end

    def block_arg
      if pass_block
        ', block'
      end
    end

    def as_method_name
      "as_#{cpp_class.sub(/Value/, '').downcase}"
    end

    def return_code
      if return_type == :bool
        'if (return_value) { return NAT_TRUE; } else { return NAT_FALSE; }'
      else
        'return return_value;'
      end
    end

    def max_argc
      if Range === argc
        argc.end
      else
        argc
      end
    end

    def generate_name
      @name = "#{cpp_class}_#{cpp_method}_binding"
    end
  end
end

puts '// DO NOT EDIT THIS FILE BY HAND!'
puts '// This file is generated by the lib/natalie/compiler/binding_gen.rb script.'
puts '// Run `make src/bindings.cpp` to regenerate this file.'
puts
puts '#include "natalie.hpp"'
puts
puts 'namespace Natalie {'
puts

gen = BindingGen.new

gen.binding('Float', '%', 'FloatValue', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '*', 'FloatValue', 'mul', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '**', 'FloatValue', 'pow', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '+', 'FloatValue', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '+@', 'FloatValue', 'uplus', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('Float', '-', 'FloatValue', 'sub', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '-@', 'FloatValue', 'uminus', argc: 0, pass_env: false, pass_block: false, return_type: :Value)
gen.binding('Float', '/', 'FloatValue', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '<', 'FloatValue', 'lt', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Float', '<=', 'FloatValue', 'lte', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Float', '<=>', 'FloatValue', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', '==', 'FloatValue', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Float', '===', 'FloatValue', 'eq', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Float', '>', 'FloatValue', 'gt', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Float', '>=', 'FloatValue', 'gte', argc: 1, pass_env: true, pass_block: false, return_type: :bool)
gen.binding('Float', 'abs', 'FloatValue', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'ceil', 'FloatValue', 'ceil', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'coerce', 'FloatValue', 'coerce', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'divmod', 'FloatValue', 'divmod', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'eql?', 'FloatValue', 'eql', argc: 1, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Float', 'fdiv', 'FloatValue', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'finite?', 'FloatValue', 'is_finite', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Float', 'floor', 'FloatValue', 'floor', argc: 0..1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'infinite?', 'FloatValue', 'is_infinite', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'inspect', 'FloatValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'nan?', 'FloatValue', 'is_nan', argc: 0, pass_env: false, pass_block: false, return_type: :bool)
gen.binding('Float', 'quo', 'FloatValue', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'to_i', 'FloatValue', 'to_i', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'to_s', 'FloatValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value)
gen.binding('Float', 'zero?', 'FloatValue', 'is_zero', argc: 0, pass_env: false, pass_block: false, return_type: :bool)

gen.binding('Integer', '%', 'IntegerValue', 'mod', argc: 1, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', '&', 'IntegerValue', 'bitwise_and', argc: 1, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', '*', 'IntegerValue', 'mul', argc: 1, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', '**', 'IntegerValue', 'pow', argc: 1, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', '+', 'IntegerValue', 'add', argc: 1, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', '-', 'IntegerValue', 'sub', argc: 1, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', '/', 'IntegerValue', 'div', argc: 1, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', '<=>', 'IntegerValue', 'cmp', argc: 1, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', '===', 'IntegerValue', 'eqeqeq', argc: 1, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', 'abs', 'IntegerValue', 'abs', argc: 0, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', 'chr', 'IntegerValue', 'chr', argc: 0, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', 'coerce', 'IntegerValue', 'coerce', argc: 1, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', 'eql?', 'IntegerValue', 'eql', argc: 1, pass_env: true, pass_block: false, return_type: :bool);
gen.binding('Integer', 'inspect', 'IntegerValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', 'succ', 'IntegerValue', 'succ', argc: 0, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', 'times', 'IntegerValue', 'times', argc: 0, pass_env: true, pass_block: true, return_type: :Value);
gen.binding('Integer', 'to_i', 'IntegerValue', 'to_i', argc: 0, pass_env: false, pass_block: false, return_type: :Value);
gen.binding('Integer', 'to_s', 'IntegerValue', 'to_s', argc: 0, pass_env: true, pass_block: false, return_type: :Value);
gen.binding('Integer', '|', 'IntegerValue', 'bitwise_or', argc: 1, pass_env: true, pass_block: false, return_type: :Value);

gen.init

puts
puts '}'
