# node-rapidjson

A Node.js package that allows you to use [RapidJSON](https://github.com/miloyip/rapidjson) for faster JSON operations.

Note that this was experimental work… it turns out that you’re better off using the normal Node.js/V8 implementation unless you’re operating on huge JSON.

There’s potential for this to be way faster, but unfortunately the bridging from V8 to C++ is a bit too costly at this stage.
