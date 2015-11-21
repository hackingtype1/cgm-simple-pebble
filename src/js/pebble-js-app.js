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
            console.log("share mode");
            share(options);
            break;
			
		case "Carelink":
            options.id = id;
			console.log("carelink mode");
            carelink(options);
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

var DEFAULT_MAX_RETRY_DURATION = 512;
//var CARELINK_SECURITY_URL = 'https://carelink.minimed.com/patient/j_security_check';
var CARELINK_SECURITY_URL = 'https://carelink.minimed.com/patient/main/login.do';
var CARELINK_AFTER_LOGIN_URL = 'https://carelink.minimed.com/patient/main/login.do';
var CARELINK_JSON_BASE_URL = 'https://carelink.minimed.com/patient/connect/ConnectViewerServlet?cpSerialNumber=NONE&msgType=last24hours&requestTime=';
var CARELINK_LOGIN_COOKIE = '_WL_AUTHCOOKIE_JSESSIONID';

//parse and use standard Carelink data
function carelink(options) {
		
	authenticateCarelink(options);
    
}

function authenticateCarelink(options){
	
    var http = new XMLHttpRequest();

    http.open("POST", CARELINK_SECURITY_URL + '?j_password=' + options.password + '&j_username=' + options.accountName);
	http.setRequestHeader("Host", 'carelink.minimed.com');
    http.setRequestHeader("Connection", 'keep-alive');
    http.setRequestHeader('Accept','text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8');
    http.setRequestHeader("User-Agent", 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10.10; rv:41.0) Gecko/20100101 Firefox/41.0');
    http.setRequestHeader("Accept-Encoding", 'gzip,deflate,sdch');
    http.setRequestHeader('Accept-Language', 'en-US,en;q=0.8');
    
    http.onload = function (e) { 
		console.log(http.status);
		var cookie = http.getResponseHeader("Set-Cookie");  
        console.log("cookie " + cookie);
        
        getCarelinkData(cookie);
    };

    http.send();
	
}

function getCarelinkData(cookie){
    var http = new XMLHttpRequest();

    http.open("GET", CARELINK_JSON_BASE_URL + Date.now());
	http.setRequestHeader("Host", 'carelink.minimed.com');
    http.setRequestHeader("User-Agent", 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10.10; rv:41.0) Gecko/20100101 Firefox/41.0');
    http.setRequestHeader("Accept-Encoding", 'gzip,deflate,sdch');
    http.setRequestHeader('Cookie', cookie);
    http.setRequestHeader('Content-Type', 'application/json');
    
    http.onload = function (e) { 
        console.log(http.status);
		console.log("response: " + http.responseBody);	 
    };

    http.send();
    
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
    http.open("POST", url , true);
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
                    "alert": 4,	
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
                if (data[0].Value < 40 * options.conversion) {
                    egv = "low";
                    delta = "check bg";
                    trend = 0;
                } else if (data[0].Value > 400 * options.conversion) {
                    egv = "hgh";
                    delta = "check bg";
                    trend = 0;
                } else {
                    var convertedEgv = (data[0].Value * options.conversion);
                    egv = (convertedEgv < 39 * options.conversion) ? parseFloat(Math.round(convertedEgv * 100) / 100).toFixed(1).toString() : convertedEgv.toString();
                    delta = (convertedEgv < 39 * options.conversion) ? parseFloat(Math.round(convertedDelta * 100) / 100).toFixed(1) : convertedDelta;
                    var deltaString = (delta > 0) ? "+" + delta.toString() : delta.toString();
					delta = deltaString + options.unit;
                    trend = (data[0].Trend > 7) ? 0 : data[0].Trend;
                }
                
                var timeDeltaMinutes = Math.floor(timeAgo / 60000);
                
                //Manage OLD data
                if (timeDeltaMinutes >= 15) {
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
				console.log("temp vibe: " + options.temp_vibe)
                MessageQueue.sendAppMessage({
                    "delta": delta,
                    "egv": egv,	
                    "trend": trend,	
                    "alert": alert,	
                    "vibe": options.temp_vibe,
                    "id": wall.toString(),
                    "time_delta_int": timeDeltaMinutes,
                });
				options.id = wall.toString();
                window.localStorage.setItem('cgmPebble_test', JSON.stringify(options));
            }

        } else {
            MessageQueue.sendAppMessage({
                "delta": "rsp-error",
                "egv": "",
                "trend": 0,
                "alert": 4,
                "vibe": 0,
                "id": now.getTime(),
                "time_delta_int": 0,
            });
        }
    };

    http.send();
	
	http.onerror = function () {
        var now = new Date();
        var id_time = now.getTime();
        MessageQueue.sendAppMessage({
            "vibe": 1,
            "egv": 0,
            "trend": 0,
            "alert": 4,
            "delta": "net-err",
            "id": id_time,
            "time_delta_int": 6,
        });
    };
   http.ontimeout = function () {
        var now = new Date();
        var id_time = now.getTime();
        MessageQueue.sendAppMessage({
            "vibe": 1,
            "egv": 0,
            "trend": 0,
            "alert": 4,
            "delta": "tout-err",
            "id": id_time,
            "time_delta_int": 6,
        });
	   share(options);
    };
}

function calculateShareAlert(egv, currentId, options) {
	console.log("egv: " + egv);
	console.log("currentId: " + currentId);
	console.log("lastId: " + options.id);
	console.log("options: " + options);

    if (parseInt(options.id, 10) == parseInt(currentId, 10)) {
        
		options.temp_vibe = 0;
		
		if (egv <= options.low)
        	return 2;
		else if (egv >= options.high)
        	return 1;
		else
        	return 0;
    } 
	options.temp_vibe = parseInt(options.vibe,10) + 1; 
    if (egv <= options.low) {
		return 2;
	}
        
    if (egv >= options.high) {
		return 1;
	}
        
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
             	
				var alert = calculateShareAlert(parseInt(data.bgs[0].sgv), data.bgs[0].datetime.toString(), options);
				console.log("alert: " + alert);
				console.log("vibe: " + options.vibe);
				
				//Manage OLD data
                if (timeDeltaMinutes >= 15) {
					var temp_alert = 0;
					if (timeDeltaMinutes % 5 == 0) {
						temp_alert = 1;
					}
					console.log("temp alert: " + temp_alert)
					MessageQueue.sendAppMessage({
                    "delta": "no data", 	//str
                    "egv": "old",		//str	
                    "trend": 0,	//int
                    "alert": 4,	//int
                    "vibe": temp_alert,
                    "id": data.bgs[0].datetime.toString(),
                    "time_delta_int": timeDeltaMinutes,
                });
				
                } else {
				
					var egv = data.bgs[0].sgv.toString();
                    //Manage HIGH & LOW
                    if (data.bgs[0].sgv <= 39 * options.conversion) {
                        egv = "low";
                        delta = "check bg";
                        trend = 0;
                    } else if (data.bgs[0].sgv >= 400 * options.conversion) {
                        egv = "hgh";
                        delta = "check bg";
                        trend = 0;
                    }
                    if (data.bgs[0].sgv < (30 * options.conversion)) {
                        egv = "???"
                    }
				
					var deltaString = data.bgs[0].bgdelta.toString() ? ("+" + data.bgs[0].bgdelta.toString()) : (data.bgs[0].bgdelta.toString());
					
				console.log("units 2: " + options.unit)
                MessageQueue.sendAppMessage({
                    "delta": deltaString + options.unit, 	//str
                    "egv": egv,		//str	
                    "trend": data.bgs[0].trend,	//int
                    "alert": alert,	//int
                    "vibe": options.temp_vibe,
                    "id": data.bgs[0].datetime.toString(),
                    "time_delta_int": timeDeltaMinutes,
                });
				}
                options.id = data.bgs[0].datetime.toString();
                window.localStorage.setItem('cgmPebble_test', JSON.stringify(options));
            } else {

                MessageQueue.sendAppMessage({
                    "delta": 'rsp-err', 	//str
                    "egv": 0,		//str	
                    "trend": 0,	//int
                    "alert": 4,	//int
                    "vibe": 1,
                    "id": id_time,
                    "time_delta_int": 6,
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
            "time_delta_int": 6,
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
            "time_delta_int": 6,
        });
    };

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
        {   'mode': 'Unset' ,
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