var http = require('http'),
      fs = require('fs'),
     url = require('url');
     
var port = 1337;

var jsonStore = {"timeEvents":[{"name":"WE Hoch","days":96,"out":1023,"event":"hoch","secoundOfDay":34200,"id":0}
,{"name":"WE Runter","days":96,"event":"runter","out":1023,"secoundOfDay":64800,"id":1}
,{"name":"Wochentags Hoch","days":31,"event":"hoch","out":1023,"secoundOfDay":27000,"id":2}
,{"name":"Wochentags Runter","days":31,"event":"runter","out":1023,"secoundOfDay":64800,"id":3}
,{"name":"WE Tueren","days":96,"event":"runter","out":1023,"secoundOfDay":61200,"id":4}
,{"name":"Wochentags Tueren","days":31,"event":"runter","out":1023,"secoundOfDay":61200,"id":5}
],
"time":{"secoundsOfDay":0,"weekDay":0},
"outputs":[{"name":"Wohnzimmmer rechts","maxTime":3500,"id":5,"state":0}
,{"name":"Gaeste WC","maxTime":2500,"state":"oben"}
,{"name":"Kueche Fenster","maxTime":2500,"state":"oben"}
,{"name":"Technik","maxTime":2500,"state":"unten"}
,{"name":"Wohnzimmmer links","maxTime":3500,"state":"unten"}
,{"name":"Kueche Tuer","maxTime":3500,"state":"unten"}
,{"name":"Eltern","maxTime":2500,"state":"gestoppt"}
,{"name":"Kind rechts","maxTime":2500,"state":"f&auml;hrt hoch"}
,{"name":"Kind links","maxTime":2500,"state":"f&auml;hrt runter"}
,{"name":"Bad","maxTime":2500,"state":9}]};

// Uhrzeit bearbeiten
setInterval(function() {
  	var t = jsonStore.time;
  	t.secoundsOfDay++;
	if (t.secoundsOfDay >= 60*60*24) {
		t.secoundsOfDay = 0;
		t.weekDay++; 
	}
	if (t.weekDay >= 7)
		t.weekDay = 0; 
}, 1000);

http.createServer(function(request, response){
    var path = url.parse(request.url).pathname;
    if (path == "/ajax"){
        console.log(request.url);
        var query = url.parse(request.url,true).query;
        if (query.cmd == 'setTime') {
			if (query.sod < 60*60*24 && query.sod >= 0 && 
		    	query.weekDay >= 0 && query.weekDay < 7) {
			    jsonStore.time.secoundsOfDay = query.sod; 
				jsonStore.time.weekDay = query.weekDay;
			} else {
				console.log("Error in setTime string" + request.url);
			}
		}
        response.writeHead(200, {"Content-Type": "application/json"});
        response.end(JSON.stringify(jsonStore));
    } else {
        fs.readFile('./client.html', function(err, file) {  
            if (err) {  
                // write an error response or nothing here  
                return;  
            }  
            response.writeHead(200, { 'Content-Type': 'text/html' });  
            response.end(file, "utf-8");  
        });
    }
}).listen(port);
console.log("server initialized on port " + port);
