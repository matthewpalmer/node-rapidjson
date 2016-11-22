var bindings = require('bindings')('node-rapidjson.node');
var fs = require('fs');

var input = fs.readFileSync(__dirname + '/simple.json').toString();

console.time('regular-parse');
for (var i = 0; i < 1000; i++) {
	JSON.parse(input);
}
console.timeEnd('regular-parse');

var c = 0;
console.time('rapid-parse');
for (var i = 0; i < 1000; i++) {
	bindings.parse(input, input.length, (result) => {
		c++;
	})
}

setInterval(() => {
	if (c == 1000) {
		console.timeEnd('rapid-parse');
		process.exit()
	}
}, 20);
