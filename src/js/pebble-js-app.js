function fetchCgmData(id) {

    var options = JSON.parse(window.localStorage.getItem('cgmPebble')) || 
        {   'mode': 'Share' ,
            'high': 180,
            'low' : 80,
            'unit': 'mg/dL',
            'accountName': '',
            'password': '' ,
            'api' : '',
            'vibe' : 1,
        };

    switch (options.mode) {
        case "Rogue":
            options.id = id;
            rogue(options);
            break;

        case "Nightscout":
            nightscout(options);
            break;

        case "Share":
            options.id = id;
            share(options);
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
                Pebble.sendAppMessage({
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
    
    http.onerror = function () {
        var now = new Date();
        var id_time = now.getTime();
        Pebble.sendAppMessage({
            "vibe": 1, 	
            "egv": 0,			
            "trend": 0,	
            "alert": 4,	
            "delta": "net error",
            "id": id_time,
            "time_delta_int": 0,
        });
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
                
                Pebble.sendAppMessage({
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
                    delta = "no data";
                    trend = 0;
                    egv = "old";
                }
                
                Pebble.sendAppMessage({
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
            Pebble.sendAppMessage({
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
    
   http.onerror = function () {
        var now = new Date();
        var id_time = now.getTime();
        Pebble.sendAppMessage({
            "vibe": 1, 	
            "egv": 0,	
            "trend": 0,	
            "alert": 4,	
            "delta": "net error",
            "id": id_time,
            "time_delta_int": 0,
        });
    };

    http.send();
}

function calculateShareAlert(egv, currentId, options) {

    if (parseInt(options.id, 10) == parseInt(currentId, 10)) {
        options.vibe = 0;
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
                




            } else {
                
                
                
                
            }
        } else {




        }
    };
    req.send(null);
}

Pebble.addEventListener("showConfiguration", function () {
    Pebble.openURL('http://cgmwatch.azurewebsites.net/config.html');
});

Pebble.addEventListener("webviewclosed", function (e) {
    var options = JSON.parse(decodeURIComponent(e.response));
    window.localStorage.setItem('cgmPebble', JSON.stringify(options));
    fetchCgmData(99);
});

Pebble.addEventListener("ready",
    function (e) {
        fetchCgmData(99);
    });

Pebble.addEventListener("appmessage",
    function (e) {
        fetchCgmData(e.payload.id);
    });