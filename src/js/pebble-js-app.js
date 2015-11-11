function fetchCgmData(id) {

    var options = JSON.parse(window.localStorage.getItem('cgmPebble_test')) || 
        {   'mode': 'Unset',
            'high': 180,
            'low' : 80,
            'unit': 'mg/dL',
            'accountName': '',
            'password': '' ,
            'api' : '',
            'vibe' : 1
        };

    switch (options.mode) {
        case "Rogue":
            options.id = id;
            rogue(options);
            break;

        case "Nightscout":
			options.id = id;
			console.log("nightscout mode");
            nightscout(options);
            break;

        case "Share":
            options.id = id;
            share(options);
            break;
			
		case "Unset":
			Pebble.sendAppMessage({
                "delta": "settings??",
                "egv": "",
                "trend": 0,
                "alert": 5,
                "vibe": 0,
                "id": 0,
                "time_delta_int": 0,
            });
			break;
    }
}

//parse and use standard NS data
function nightscout(options) {

}

//use D's share API------------------------------------------//
function share(options) {

    if (options.unit == "mgdl")
    {
        options.conversion = 1;
        options.unit = "mg/dL";
        
    } else if (options.unit == "mg/dL") {
		options.conversion = 1;
        options.unit = "mg/dL";	
	} else {
        options.conversion = .0555;       
        options.unit = "mmol/l";
    }
    
    //convert, set data types
    //options.high = parseInt(options.high, 10) * options.conversion;
    //options.low = parseInt(options.low, 10) * options.conversion;
    options.vibe = parseInt(options.vibe, 10);
   
    var defaults = {
        "applicationId": "d89443d2-327c-4a6f-89e5-496bbb0317db"
        , "agent": "Dexcom Share/3.0.2.11 CFNetwork/711.2.23 Darwin/14.0.0"
        , login: 'https://share1.dexcom.com/ShareWebServices/Services/General/LoginPublisherAccountByName'
        , accept: 'application/json'
        , 'content-type': 'application/json'
        , LatestGlucose: "https://share1.dexcom.com/ShareWebServices/Services/Publisher/ReadPublisherLatestGlucoseValues"
    };

    authenticateShare(options, defaults);
}

function authenticateShare(options, defaults) {   
 
    var body = {
        "password": options.password
        , "applicationId": options.applicationId || defaults.applicationId
        , "accountName": options.accountName
    };

    var http = new XMLHttpRequest();
    var url = defaults.login;
    http.open("POST", url, true);
    http.setRequestHeader("User-Agent", defaults.agent);
    http.setRequestHeader("Content-type", defaults['content-type']);
    http.setRequestHeader('Accept', defaults.accept);
    
    var data;
    http.onload = function (e) {
        if (http.status == 200) {
            data = getShareGlucoseData(http.responseText.replace(/['"]+/g, ''), defaults, options);
        } else {
            console.log("login error");
                var now = new Date();
                var id_time = now.getTime();
                MessageQueue.sendAppMessage({
                    "vibe": 1, 	
                    "egv": 0,		
                    "trend": 0,	
                    "alert": 4,
                    "delta": "login error",
                    "id": id_time,
                    "time_delta_int": 0,
                });
            
        }
    };

    http.send(JSON.stringify(body));

}

function getShareGlucoseData(sessionId, defaults, options) {
    var now = new Date();
    var http = new XMLHttpRequest();
    var url = defaults.LatestGlucose + '?sessionID=' + sessionId + '&minutes=' + 1440 + '&maxCount=' + 2;
    http.open("POST", url, true);

    //Send the proper header information along with the request
    http.setRequestHeader("User-Agent", defaults.agent);
    http.setRequestHeader("Content-type", defaults['content-type']);
    http.setRequestHeader('Accept', defaults.accept);
    http.setRequestHeader('Content-Length', 0);

    http.onload = function (e) {
             
        if (http.status == 200) {
            var data = JSON.parse(http.responseText);
            
            //handle arrays less than 2 in length
            if (data.length == 0) {
                
                MessageQueue.sendAppMessage({
                    "delta": "data error", 	
                    "egv": "",			
                    "trend": 0,	
                    "alert": 5,	
                    "vibe": 0,
                    "id": now.getTime(),
                    "time_delta_int": 0,
                });
            } else { 
            
                //TODO: calculate loss
                var regex = /\((.*)\)/;
                var wall = parseInt(data[0].WT.match(regex)[1]);
                var timeAgo = now.getTime() - wall;       
                var alert = calculateShareAlert(data[0].Value, wall.toString(), options);
                var egv, delta, trend, convertedDelta;

                if (data.length == 1) {
                    delta = "can't calc";
                } else {
                    var deltaZero = data[0].Value * options.conversion;
                    var deltaOne = data[1].Value * options.conversion;
                    convertedDelta = (deltaZero - deltaOne);
                }

                //Manage HIGH & LOW
                if (data[0].Value < 40) {
                    egv = "low";
                    delta = "check bg";
                    trend = 0;
                } else if (data[0].Value > 400) {
                    egv = "hgh";
                    delta = "check bg";
                    trend = 0;
                } else {
                    var convertedEgv = (data[0].Value * options.conversion);
                    egv = (convertedEgv < 39) ? parseFloat(Math.round(convertedEgv * 100) / 100).toFixed(1).toString() : convertedEgv.toString();
                    delta = (convertedEgv < 39) ? parseFloat(Math.round(convertedDelta * 100) / 100).toFixed(1) : convertedDelta;
                    delta = delta.toString() + options.unit;
                    trend = (data[0].Trend > 7) ? 0 : data[0].Trend;
                }
                
                var timeDeltaMinutes = Math.floor(timeAgo / 60000);
                
                //Manage OLD data
                if (timeDeltaMinutes > 15) {
					var temp_alert = 0;
					if (timeDeltaMinutes % 5 == 0) {
						temp_alert = 1;
					}
                    delta = "no data";
                    trend = 0;
                    egv = "old";
					options.vibe = temp_alert;
					alert = 4;
                }
                
                MessageQueue.sendAppMessage({
                    "delta": delta,
                    "egv": egv,	
                    "trend": trend,	
                    "alert": alert,	
                    "vibe": options.vibe,
                    "id": wall.toString(),
                    "time_delta_int": timeDeltaMinutes,
                });
            }

        } else {
            MessageQueue.sendAppMessage({
                "delta": "data error",
                "egv": "",
                "trend": 0,
                "alert": 5,
                "vibe": 0,
                "id": now.getTime(),
                "time_delta_int": 0,
            });
        }
    };

    http.send();
}

function calculateShareAlert(egv, currentId, options) {
	console.log("egv: " + egv);
	console.log("currentId: " + currentId);
	console.log("options: " + options);

    if (parseInt(options.id, 10) == parseInt(currentId, 10)) {
        
		options.vibe = 0;
		
		if (egv <= options.low)
        	return 2;
		else if (egv >= options.high)
        	return 1;
		else
        	return 3;
    }

    if (egv <= options.low)
        return 2;

    if (egv >= options.high)
        return 1;
        
	//new EGV, but within range
    return 0;
}



//---------Nightscout---------------------//
function nightscout(options) {
	
	var req = new XMLHttpRequest();
	var now = new Date();
    var id_time = now.getTime();
	
	if (options.unit == "mgdl")
    {
        options.conversion = 1;
        options.unit = "mg/dL";
        
    } else if (options.unit == "mg/dL") {
		options.conversion = 1;
        options.unit = "mg/dL";	
	} else {
        options.conversion = .0555;       
        options.unit = "mmol/l";
    }
	
    req.open('GET', options.api, true);

    req.onload = function (e) {
        console.log(req.readyState);
        if (req.readyState == 4) {
            console.log(req.status);
            if (req.status == 200) {
                console.log("text: " + req.responseText);
				var data = JSON.parse(req.responseText);
				var timeAgo = now.getTime() - data.bgs[0].datetime;
				var timeDeltaMinutes = Math.floor(timeAgo / 60000);
             	
				var alert = calculateNSAlert(parseInt(data.bgs[0].sgv), data.bgs[0].datetime.toString(), options);
				console.log("alert: " + alert);
				console.log("vibe: " + options.vibe);
				
				//Manage OLD data
                if (timeDeltaMinutes > 15) {
					var temp_alert = 0;
					if (timeDeltaMinutes % 5 == 0) {
						temp_alert = 1;
					}
					console.log("temp alert: " + temp_alert)
					MessageQueue.sendAppMessage({
                    "delta": "no data", 	//str
                    "egv": "old",		//int	
                    "trend": 0,	//int
                    "alert": 4,	//int
                    "vibe": temp_alert,
                    "id": data.bgs[0].datetime.toString(),
                    "time_delta_int": timeDeltaMinutes,
                });
				
                } else {
				
				
                MessageQueue.sendAppMessage({
                    "delta": data.bgs[0].bgdelta.toString() + options.unit, 	//str
                    "egv": data.bgs[0].sgv,		//int	
                    "trend": data.bgs[0].trend,	//int
                    "alert": alert,	//int
                    "vibe": options.vibe,
                    "id": data.bgs[0].datetime.toString(),
                    "time_delta_int": timeDeltaMinutes,
                });
				}
                options.id = data.bgs[0].datetime.toString();
                window.localStorage.setItem('cgmPebble_test', JSON.stringify(options));
            } else {

                MessageQueue.sendAppMessage({
                    "delta": 'error', 	//str
                    "egv": 0,		//int	
                    "trend": 0,	//int
                    "alert": 4,	//in
                    "vibe": 1,
                    "id": id_time,
                    "time_delta_int": 0,
                });
                console.log("first if");
                console.log(req.status);
            }
        } else {
            console.log("second if");
        }
    };
    req.send(null);

	req.onerror = function () {
        var now = new Date();
        var id_time = now.getTime();
        MessageQueue.sendAppMessage({
            "vibe": 1,
            "egv": 0,
            "trend": 0,
            "alert": 4,
            "delta": "net-err",
            "id": id_time,
            "time_delta_int": 0,
        });
    };
   req.ontimeout = function () {
        var now = new Date();
        var id_time = now.getTime();
        MessageQueue.sendAppMessage({
            "vibe": 1,
            "egv": 0,
            "trend": 0,
            "alert": 4,
            "delta": "tout-err",
            "id": id_time,
            "time_delta_int": 0,
        });
    };

}

function calculateNSAlert(egv, currentId, options) {
	console.log("egv: " + egv);
	console.log("currentId: " + currentId);
	console.log("options.id: " + options.id);
	console.log("low: " + options.low);
	console.log("high: " + options.high);

 	if (parseInt(options.id, 10) == parseInt(currentId, 10)) {
        
		options.vibe = 0;
		
		if (egv <= options.low)
        	return 2;
		else if (egv >= options.high)
        	return 1;
		else
        	return 3;
    }

    if (egv <= options.low)
        return 2;

    if (egv >= options.high)
        return 1;
        
	//new EGV, but within range
    return 0;

}


//using something different? code it up here-------ROGUE-----------------------------//:
function rogue(options) {
var response;
    var req = new XMLHttpRequest();
    req.open('GET', options.api, true);

    req.onload = function (e) {
        console.log(req.readyState);
        if (req.readyState == 4) {
            console.log(req.status);
            if (req.status == 200) {
                console.log("text: " + req.responseText);
                response = JSON.parse(req.responseText);
                options.wt = response[0].datetime;
                options.egv = response[0].sgv;
                var alert = 0;
				var egv = response[0].sgv;
                if (response[0].noise != "Clean")
                    egv = response[0].calculatedBg.toString();

                var delta = Math.round(response[0].timesinceread) + " min. ago";
                if (Math.round(response[0].timesinceread) < 1)
                    delta = "now";
                    
                if  (response[0].trend == 4)
                     response[0].trend = 0;
                else if  (response[0].trend < 4)
                     response[0].trend = response[0].trend + 1;     
                     
                var trend = (response[0].trend > 7) ? 0 : response[0].trend;       

                MessageQueue.sendAppMessage({
                    "delta": response[0].bgdelta.toString() + "mg/dL", 	//str
                    "egv": egv,		//int	
                    "trend": trend,	//int
                    "alert": alert,	//int
                    "vibe": parseInt(options.vibe_temp,10),
                    "id": response[0].id.toString(),
                    "time_delta_int": Math.floor(response[0].timesinceread),
                });
                options.id = response[0].id.toString();
                window.localStorage.setItem('cgmPebble_test', JSON.stringify(options));
            } else {
                Pebble.sendAppMessage({
                    "delta": 'must code.', 	//str
                    "egv": 0,		//int	
                    "trend": 0,	//int
                    "alert": 4,	//in
                    "vibe": 1,
                    "id": response[0].id.toString(),
                    "time_delta_int": 0,
                });
                console.log("first if");
                console.log(req.status);
            }
        } else {
            console.log("second if");
        }
    };
    req.send(null);
}

Pebble.addEventListener("showConfiguration", function () {
    Pebble.openURL('http://cgmwatch.azurewebsites.net/config.1.html');
});

Pebble.addEventListener("webviewclosed", function (e) {
    var options = JSON.parse(decodeURIComponent(e.response));
    window.localStorage.setItem('cgmPebble_test', JSON.stringify(options));
    fetchCgmData(99);
});

Pebble.addEventListener("ready",
    function (e) {
        var options = JSON.parse(window.localStorage.getItem('cgmPebble_test')) || 
        {   'mode': 'Share' ,
            'high': 180,
            'low' : 80,
            'unit': 'mgdL',
            'accountName': '',
            'password': '' ,
            'api' : '',
            'vibe' : 1,
            'id' : 99,
        };     
        fetchCgmData(options.id);
    });

Pebble.addEventListener("appmessage",
    function (e) {
        fetchCgmData(e.payload.id);
    });

// message queue-ing to pace calls from C function on watch
var MessageQueue = (
    function () {

        var RETRY_MAX = 5;

        var queue = [];
        var sending = false;
        var timer = null;

        return {
            reset: reset,
            sendAppMessage: sendAppMessage,
            size: size
        };

        function reset() {
            queue = [];
            sending = false;
        }

        function sendAppMessage(message, ack, nack) {

            if (!isValidMessage(message)) {
                return false;
            }

            queue.push({
                message: message,
                ack: ack || null,
                nack: nack || null,
                attempts: 0
            });

            setTimeout(function () {
                sendNextMessage();
            }, 1);

            return true;
        }

        function size() {
            return queue.length;
        }

        function isValidMessage(message) {
            // A message must be an object.
            if (message !== Object(message)) {
                return false;
            }
            var keys = Object.keys(message);
            // A message must have at least one key.
            if (!keys.length) {
                return false;
            }
            for (var k = 0; k < keys.length; k += 1) {
                var validKey = /^[0-9a-zA-Z-_]*$/.test(keys[k]);
                if (!validKey) {
                    return false;
                }
                var value = message[keys[k]];
                if (!validValue(value)) {
                    return false;
                }
            }

            return true;

            function validValue(value) {
                switch (typeof (value)) {
                    case 'string':
                        return true;
                    case 'number':
                        return true;
                    case 'object':
                        if (toString.call(value) == '[object Array]') {
                            return true;
                        }
                }
                return false;
            }
        }

        function sendNextMessage() {

            if (sending) { return; }
            var message = queue.shift();
            if (!message) { return; }

            message.attempts += 1;
            sending = true;
            Pebble.sendAppMessage(message.message, ack, nack);

            timer = setTimeout(function () {
                timeout();
            }, 1000);

            function ack() {
                clearTimeout(timer);
                setTimeout(function () {
                    sending = false;
                    sendNextMessage();
                }, 200);
                if (message.ack) {
                    message.ack.apply(null, arguments);
                }
            }

            function nack() {
                clearTimeout(timer);
                if (message.attempts < RETRY_MAX) {
                    queue.unshift(message);
                    setTimeout(function () {
                        sending = false;
                        sendNextMessage();
                    }, 200 * message.attempts);
                }
                else {
                    if (message.nack) {
                        message.nack.apply(null, arguments);
                    }
                }
            }

            function timeout() {
                setTimeout(function () {
                    sending = false;
                    sendNextMessage();
                }, 1000);
                if (message.ack) {
                    message.ack.apply(null, arguments);
                }
            }

        }

    } ());