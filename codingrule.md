# Coding Rule

## Conditional branch
- If the branch has `else` clause, corresponding `if` should be used with **positive** condition.

- Good example:

```cpp
#if defined(ASYNC_MQTT_VAL1)
    // do some on defined
#else  // defined(ASYNC_MQTT_VAL1)
    // do some on not defined
#endif // defined(ASYNC_MQTT_VAL1)

if (val1 == 10) {
    // do some on val1 == 10
}
else {
    // do some on val1 != 10
}
```

- Bad example

```cpp
#if !defined(ASYNC_MQTT_VAL1)
    // do some on not defined
#else  // !defined(ASYNC_MQTT_VAL1)
    // do some on defined
#endif // !defined(ASYNC_MQTT_VAL1)

if (val1 != 10) {
    // do some on val1 != 10
}
else {
    // do some on val1 == 10
}
```

## Include guard

For file `include/async_mqtt/file.hpp`
```cpp
#if !defined(ASYNC_MQTT_FILE_HPP)
#define ASYNC_MQTT_FILE_HPP

#endif // ASYNC_MQTT_FILE_HPP
```

For file `include/async_mqtt/sub1/file.hpp`
```cpp
#if !defined(ASYNC_MQTT_SUB1_FILE_HPP)
#define ASYNC_MQTT_SUB1_FILE_HPP

#endif // ASYNC_MQTT_SUB1_FILE_HPP
```

- Use `#if !defined()`
- Add comment after `#endif`. It doesn't contain `!defined()`.
  - It is a special rule for include guard.

## Macro

### Values
- The name of the values must be start with `ASYNC_MQTT_`.

### Conditional branch

- Don't use `#ifdef` and `#ifndef`, use `#if defined(...)`.
- Add comment after `#else  ` and `#endif`. For `#else` adiitional one whitespace is required. The contents of the comment should be the same as `#if` condition.
- Good example:
```cpp
#if defined(ASYNC_MQTT_VAL1)
#else  // defined(ASYNC_MQTT_VAL1)
#endif // defined(ASYNC_MQTT_VAL1)
```

## namespace
- `namespace` doesn't introduce indent.
- After the comment end of the namespace `}` , the contnents of the comment is `namespace ns`.
- Good example:
```cpp
namespace async_mqtt {
// ...
namespace detail {

struct foo;

} // namespace detail

} // namespace async_mqtt
```

