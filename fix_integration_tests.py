#!/usr/bin/env python3
import re

# Read the file
with open('test/implicit_integration_test.cpp', 'r') as f:
    content = f.read()

# Pattern 1: Single line CreateScalarVariable assignments for ComputeResiduals/Gradients
# vars["x"] = CreateScalarVariable(3.0);
# Should become:
# vars["x"] = Variable(client.GetVariableMeta("x")); vars["x"](0) = 3.0;

def fix_scalar_assignment(match):
    indent = match.group(1)
    var_name = match.group(2)
    value = match.group(3)
    return f'{indent}vars["{var_name}"] = Variable(client.GetVariableMeta("{var_name}"));\n{indent}vars["{var_name}"](0) = {value};'

# Fix patterns like: vars["x"] = CreateScalarVariable(5.0);
content = re.sub(
    r'(\s+)vars\["(\w+)"\]\s*=\s*CreateScalarVariable\(([\d.]+)\);',
    fix_scalar_assignment,
    content
)

# Pattern 2: Vector assignments
# vars["x"] = CreateVectorVariable({1.0, 2.0, 3.0});
# Should become:
# vars["x"] = Variable(client.GetVariableMeta("x"));
# for (size_t i = 0; i < 3; ++i) vars["x"](i) = values[i];

def fix_vector_assignment(match):
    indent = match.group(1)
    var_name = match.group(2)
    values_str = match.group(3)
    values = [v.strip() for v in values_str.split(',')]

    result = f'{indent}vars["{var_name}"] = Variable(client.GetVariableMeta("{var_name}"));\n'
    for i, val in enumerate(values):
        result += f'{indent}vars["{var_name}"]({i}) = {val};\n'
    return result.rstrip('\n') + ';'  # Remove extra newline, add semicolon

# This pattern is complex, let's handle it differently
# For now, let's just fix the scalar ones

# Write the file
with open('test/implicit_integration_test.cpp', 'w') as f:
    f.write(content)

print("Fixed integration tests")
