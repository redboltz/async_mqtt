# C++20 coroutine
C++20 coroutine is stackful coroutine that is supported by the language.
See https://en.cppreference.com/w/cpp/language/coroutines

C++coroutine coroutine can avoid deeply nested callbacks. The code becomes easy to read.

## co_spawn your application routine

```cpp
int main(int argc, char* argv[]) {
    am::setup_log(am::severity_level::info);
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    as::io_context ioc;
    as::co_spawn(ioc, proc(ioc.get_executor(), argv[1], argv[2]), as::detached);
    ioc.run();
}
```

In order to use coroutine, you need to call `as::co_spawn()` to launch the coroutine as follows:

```cpp
as::co_spawn(ioc, proc(ioc.get_executor(), argv[1], argv[2]), as::detached);
```

## define application routine

Define application routine `proc()` as follows. The return value should be wrapped `as::awaitable<>`.

```cpp
template <typename Executor>
as::awaitable<void>
proc(Executor exe, std::string_view host, std::string_view port) {
   ...
```

### define endpoint

```cpp
    as::ip::tcp::socket resolve_sock{exe};
    as::ip::tcp::resolver res{exe};
    am::endpoint<am::role::client, am::protocol::mqtt> amep {
        am::protocol_version::v3_1_1,
        exe
    };
    std::cout << "start" << std::endl;
```

There is nothing special. The same as non coroutine endpoint.

### chain async calls

```cpp
        // Resolve hostname
        auto eps = co_await res.async_resolve(host, port, as::use_awaitable);
        std::cout << "async_resolved" << std::endl;

        // Layer
        // am::stream -> TCP

        // Underlying TCP connect
        co_await as::async_connect(
            amep.next_layer(),
            eps,
            as::use_awaitable
        );
        std::cout << "TCP connected" << std::endl;

        // Send MQTT CONNECT
        if (auto se = co_await amep.send(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    am::allocate_buffer("cid1"),
                    am::nullopt, // will
                    am::nullopt, // username set like am::allocate_buffer("user1"),
                    am::nullopt  // password set like am::allocate_buffer("pass1")
                },
                as::use_awaitable
            )
        ) {
            std::cout << "MQTT CONNECT send error:" << se.what() << std::endl;
            co_return;
        }

        // Recv MQTT CONNACK
        if (am::packet_variant pv = co_await amep.recv(as::use_awaitable)) {
            pv.visit(
                am::overload {
                    [&](am::v3_1_1::connack_packet const& p) {
                        std::cout
                            << "MQTT CONNACK recv"
                            << " sp:" << p.session_present()
                            << std::endl;
                    },
                    [](auto const&) {}
                }
            );
        }
        else {
            std::cout
                << "MQTT CONNACK recv error:"
                << pv.get<am::system_error>().what()
                << std::endl;
            co_return;
        }
```

When you call async functions, add `co_await` keyword before the function call, and pass `as::use_awaitable` as CompletionToken. This is C++20 coroutine way.

For example, sending CONNECT packet as follows. When send() is called, async send procedure is triggered. When the procedure is finished, then the context is returned. You can access return value se.
If se is evalueted as true, that means some error happened. So the code printss error message and `co_return`. When you want to return from the function that has `as::awaitable<>` return value, you need to call `co_return` instead of `return`.

```cpp
        // Send MQTT CONNECT
        if (auto se = co_await amep.send(
                am::v3_1_1::connect_packet{
                    true,   // clean_session
                    0x1234, // keep_alive
                    am::allocate_buffer("cid1"),
                    am::nullopt, // will
                    am::nullopt, // username set like am::allocate_buffer("user1"),
                    am::nullopt  // password set like am::allocate_buffer("pass1")
                },
                as::use_awaitable
            )
        ) {
            std::cout << "MQTT CONNECT send error:" << se.what() << std::endl;
            co_return;
        }
```

# whole code (example)
[ep_cpp20coro_mqtt_client.cpp](../../main/example/ep_cpp20coro_mqtt_client.cpp)