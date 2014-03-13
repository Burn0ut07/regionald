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
        if (matchesCount == 4) {
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

function requestRegionPage() {
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
                if (regionList.length !== 0) {
                    Pebble.sendAppMessage({region_list: regionList.shift()}, ackHandler, nackHandler);
                } else {
                    Pebble.sendAppMessage({region_selected: regionSelected}, function() {}, nackHandler);
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

// Called when JS is ready
Pebble.addEventListener("ready",
    function(e) {
        var req = new XMLHttpRequest();
        req.open('POST', 'https://unlocator.com/account/login', true);
        req.onload = function() {
            console.log("Onload called. status: " + this.status + ", readyState: " + this.readyState);
            if (this.readyState == 4 && this.status == 200) {
                requestRegionPage();
            }
        };
        req.onerror = transferFailed;
        req.onabort = transferCanceled;
        var params = "amember_login=<login>&amember_pass=<pass>";
        req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
        req.setRequestHeader("Content-length", params.length);
        req.setRequestHeader("Connection", "close");
        req.send(params);
    }
);

// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage",
    function(e) {
        if ('region_update' in e.payload) {
            updateRegion(e.payload.region_update);
        }
    }
);