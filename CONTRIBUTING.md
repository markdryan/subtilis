# Coding Style

Subtilis uses the [linux coding standard](http://www.kernel.org/doc/Documentation/CodingStyle) with one
exception; typedefs are used for structures, for example:

```
struct subtilis_fixed_buffer_t_ {
    size_t max_size;
    size_t start;
    size_t end;
    uint8_t data[1];
};
typedef struct subtilis_fixed_buffer_t_ subtilis_fixed_buffer_t;
```

Patches should pass [checkpath.pl](https://raw.githubusercontent.com/torvalds/linux/master/scripts/checkpatch.pl)  before being submitted, which should be run as follows:

`
checkpatch.pl --no-tree -f --strict --show-types --ignore NEW_TYPEDEFS --ignore PREFER_KERNEL_TYPES --ignore SPLIT_STRING
`

Code should be formatted using clang-format.  A .clang-format file is provided in
the repository, so it's simply a matter of running

`
clang-format -i *.c *.h
`

Formatting and coding standard rules are validate by the travis builds.  Changes
will be automatically requested for any non-conforming patch.





