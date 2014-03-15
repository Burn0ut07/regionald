function transferFailed(evt) {
  console.log("An error occurred while transferring the file.");
}

function transferCanceled(evt) {
  console.log("The transfer has been canceled by the user.");
}

function extractRegionInfo(responseText, extractList) {
    var selectExtract = /<select name="netflix">\n(.*)<\/select>/g;
    var optionsExtract = /<option value="(.{2})"(?: selected)?>(.+?)<\/option>/g;
    var selectedOptionExtract = /<option value="(.{2})" selected>/g;
    var options = selectExtract.exec(responseText)[1].trim();

    // Only want to extract selected
    if (!extractList) {
        return selectedOptionExtract.exec(options)[1];
    }

    var currentMatch, allMatches = "", matchesList = [], matchesCount = 0;
    while ((currentMatch = optionsExtract.exec(options)) !== null)
    {
        if (allMatches.length > 0) {
            allMatches += ",";
        }
        currentMatch.shift();
        allMatches += currentMatch[0] + "," + currentMatch[1];
        matchesCount++;
        if (matchesCount == 6) {
            matchesList.push(allMatches);
            matchesCount = 0;
            allMatches = "";
        }
    }

    if (allMatches.length !== 0) {
        matchesList.push(allMatches);
    }

    return matchesList;
}

function requestRegions() {
    var req = new XMLHttpRequest();
    req.open('GET', 'https://unlocator.com/account/region-settings', true);
    req.onload = function() {
        console.log("RegionSettings onload called. status: " + this.status + ", readyState: " + this.readyState);   
        if (this.readyState == 4 && this.status == 200) {
            var regionList = extractRegionInfo(this.responseText, true);
            var regionSelected = extractRegionInfo(this.responseText, false);
            console.log("Region list: " + regionList);
            console.log("Region selected: " + regionSelected);
            var ackHandler = function(e) {
                console.log("Message success!");
                if (regionList.length !== 1) {
                    Pebble.sendAppMessage({region_list: regionList.shift()}, ackHandler, nackHandler);
                } else {
                    Pebble.sendAppMessage({region_list: regionList.shift(), region_selected: regionSelected}, function() {}, nackHandler);
                }
            };
            var nackHandler = function(e) {
                console.log("Message not sent sucessfully.");
                console.log("Event keys: " + Object.keys(e));
                console.log("Data: " + e.data);
                console.log("Data keys: " + Object.keys(e.data));
            };
            Pebble.sendAppMessage({region_list: regionList.shift(), region_list_begin: true}, ackHandler, nackHandler);
        }       
    };
    req.onerror = transferFailed;
    req.onabort = transferCanceled;
    req.send();
}

function isUserLoggedIn() {
    // TODO: Quickly check if user is already logged in
    return false;
}

function updateRegion(newRegion) {
    var req = new XMLHttpRequest();
    req.open('POST', 'https://unlocator.com/account/region-settings', true);
    req.onload = function() {
        if (this.readyState == 4 && this.status == 200) {
            // <div class="alert alert-success">Your Netflix region was successfully changed to <region></div>
            console.log("region updated");
            Pebble.sendAppMessage({region_selected: newRegion});
        }
    };
    req.onerror = transferFailed;
    req.onabort = transferCanceled;
    var params = "netflix=" + newRegion;
    req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    req.setRequestHeader("Content-length", params.length);
    req.setRequestHeader("Connection", "close");
    req.send(params);
}

function loginUser(username, password, afterLogin) {
    var req = new XMLHttpRequest();
    req.open('POST', 'https://unlocator.com/account/login', true);
    req.onload = function() {
        console.log("Onload called. status: " + this.status + ", readyState: " + this.readyState);
        if (this.readyState == 4 && this.status == 200) {
            if (afterLogin !== null) {
                afterLogin();
            }
        }
    };
    req.onerror = transferFailed;
    req.onabort = transferCanceled;
    var params = "amember_login=" + username + "&amember_pass=" + password;
    req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    req.setRequestHeader("Content-length", params.length);
    req.setRequestHeader("Connection", "close");
    req.send(params);
}

function getUserData() {
    // Not logged in, so lets check localStorage for credentials
    var userAuth = localStorage.getItem('user-auth');
    if (userAuth === null) {
        // If no credentials found alert watch
        sendServiceError('Setup login in settings');
        return null;
    }

    var user = JSON.parse(userAuth);
    if (user.username === '' || user.password === '') {
        sendServiceError('Invalid credentials');
        return null;
    }

    return user;
}

function configUrlWithStored(baseUrl) {
    var userAuth = localStorage.getItem('user-auth');
    if (userAuth === null) {
        return baseUrl;
    }

    var user = JSON.parse(userAuth);
    var queryParams = '';
    Object.keys(user).forEach(function (key) {
        queryParams += key + '=' + encodeURIComponent(user[key]) + '&';
    });

    return baseUrl + '?' + queryParams.slice(0, -1);
}

function sendServiceError(error) {
    Pebble.sendAppMessage({service_error: error});
}

// Called when JS is ready
Pebble.addEventListener("ready",
    function(e) {
        console.log('ready function called');

        // If already logged in request regions and send to watch
        if (isUserLoggedIn()) {
            requestRegions();
            return;
        }

        var user = getUserData();
        if (user !== null) {
            loginUser(user.username, user.password, requestRegions);
        }
    }
);

Pebble.addEventListener("showConfiguration", function() {
  console.log("showing configuration");
  var configUrl = configUrlWithStored('http://burn0ut07.github.com/regionald/');
  console.log("configUrl: " + configUrl);
  Pebble.openURL(configUrl);
});

Pebble.addEventListener("webviewclosed", function(e) {
  console.log("configuration closed");
  // webview closed
  console.log('response: ' + e.response);
  if (e.response === null || e.response === '') {
    console.log("No data in form");
    return;
  }
  var user = JSON.parse(decodeURIComponent(e.response));
  localStorage.setItem('user-auth', JSON.stringify(user));

  if (getUserData()) {
    loginUser(user.username, user.password, requestRegions);
  }
});

// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",
    function(e) {
        if ('region_update' in e.payload) {
            updateRegion(e.payload.region_update);
        }
    }
);