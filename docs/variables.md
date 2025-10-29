# Working with Variables {#variables}

[TOC]

## Introduction

The philote::Variable class provides a container for multi-dimensional arrays used in Philote disciplines.
Variables can represent inputs, outputs, or residuals and support operations like segmentation,
streaming over gRPC, and element access.

## Creating Variables

### From Type and Shape

```cpp
// Create a variable with shape
philote::Variable pressure(philote::kInput, {10, 10});

// Set values
for (size_t i = 0; i < 100; ++i) {
    pressure(i) = 101325.0 + i * 10.0;
}
```

### From Metadata

```cpp
philote::VariableMetaData meta;
meta.set_name("temperature");
meta.add_shape(5);
meta.set_units("K");
meta.set_type(philote::kOutput);

philote::Variable temp(meta);
temp(0) = 300.0;
```

## Accessing Variable Data

### Element Access

```cpp
philote::Variable vec(philote::kOutput, {100});

// Set individual elements
vec(0) = 1.5;
vec(99) = 2.7;

// Get individual elements
double val = vec(0);
```

### Shape Information

```cpp
philote::Variable matrix(philote::kOutput, {3, 3});

// Get shape information
std::vector<size_t> shape = matrix.Shape();  // Returns {3, 3}
size_t total_size = matrix.Size();           // Returns 9
```

## Working with Segments

Variables support partial array access through segments:

```cpp
philote::Variable large_array(philote::kOutput, {1000});

// Set a segment
std::vector<double> data = {1.0, 2.0, 3.0, 4.0, 5.0};
large_array.Segment(10, 15, data);

// Get a segment
std::vector<double> retrieved = large_array.Segment(10, 15);
```

## Variables Collections

### Variables Map

The philote::Variables type is a map of variable names to Variable objects:

```cpp
philote::Variables inputs;
inputs["x"] = philote::Variable(philote::kInput, {1});
inputs["y"] = philote::Variable(philote::kInput, {1});

inputs.at("x")(0) = 5.0;
inputs.at("y")(0) = -2.0;
```

### Partials (Jacobians)

Partials use pairs of strings as keys (output, input):

```cpp
philote::Partials gradients;

// Traditional approach with std::make_pair
gradients[std::make_pair("f", "x")] = philote::Variable(philote::kOutput, {1});
gradients[std::make_pair("f", "x")](0) = 2.5;

// Cleaner approach with initializer list
gradients[{"f", "y"}] = philote::Variable(philote::kOutput, {1});
gradients[{"f", "y"}](0) = 3.7;
```

## PairDict for Cleaner Access

For more readable gradient access, use PairDict:

```cpp
philote::PairDict<philote::Variable> gradients;

// Create gradient variables
gradients("f", "x") = philote::Variable(philote::kOutput, {1});
gradients("f", "y") = philote::Variable(philote::kOutput, {1});

// Set gradient values
gradients("f", "x")(0) = 2.5;
gradients("f", "y")(0) = 3.7;

// Check if a gradient exists
if (gradients.contains("f", "x")) {
    double df_dx = gradients("f", "x")(0);
}
```

## Multi-dimensional Arrays

Variables support multi-dimensional arrays with row-major ordering:

```cpp
// Create a 3x3 matrix
philote::Variable matrix(philote::kOutput, {3, 3});

// Access uses linear indexing (row-major)
// Element [i,j] is at index i*cols + j
matrix(0) = 1.0;  // [0,0]
matrix(1) = 2.0;  // [0,1]
matrix(2) = 3.0;  // [0,2]
matrix(3) = 4.0;  // [1,0]
// ... and so on
```

## Best Practices

1. **Always specify units**: Provide physical units for all variables
2. **Check dimensions**: Ensure variable shapes match expectations
3. **Use descriptive names**: Variable names should be clear and meaningful
4. **Preallocate**: Create variables with correct shape before filling data
5. **Use type-safe access**: Prefer `.at()` over `[]` for bounds checking

## See Also

- @ref explicit_disciplines - Using variables in explicit disciplines
- @ref implicit_disciplines - Using variables in implicit disciplines
- philote::Variable - API documentation
- philote::PairDict - API documentation
