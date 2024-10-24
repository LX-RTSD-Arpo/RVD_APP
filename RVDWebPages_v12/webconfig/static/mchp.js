function sendRebootCommand() {
    fetch('/reboot', {
        method: 'POST',
    }).then(response => {
        alert("Reboot command sent successfully!");
    }).catch(error => {
        console.error("Error:", error);
        alert("Error sending reboot command.");
    });
}

function sendTestCommand() {
    fetch('/test-relay', {
        method: 'POST',
    }).then(response => {
        alert("Test command sent successfully!");
    }).catch(error => {
        console.error("Error:", error);
        alert("Error sending test command.");
    });
}

function ConnectCommand() {
    fetch('/ConnectCommand', {
        method: 'POST',
    }).then(response => {
        alert("Test command sent successfully!");
    }).catch(error => {
        console.error("Error:", error);
        alert("Error sending connect command.");
    });
}

async function fetchRVDData() {
	try {
		const response = await fetch('/get-firmware-detail');
		const data = await response.json();

		if (response.ok) {
			// Fill in the table with the fetched data
			document.getElementById('filename').textContent = data.filename;
			document.getElementById('version').textContent = data.version;
			document.getElementById('moddate').textContent = data.moddate;
			localStorage.setItem('firmwareName', data.filename);
		} else {
			// Display an error message if the data couldn't be retrieved
			document.getElementById('filename').textContent = 'Error';
			document.getElementById('version').textContent = data.error;
			document.getElementById('moddate').textContent = 'N/A';
		}
	} catch (error) {
		// Handle network errors or issues
		document.getElementById('filename').textContent = 'File not found';
		document.getElementById('version').textContent = 'N/A';
		document.getElementById('moddate').textContent = 'N/A';
		document.getElementById('upload-status').textContent = data.error;
	}
}

function initializeIdleLogoutHandler(idleLimit = 300) {
    let idleTime = 0;
    let confirmationShown = false; // Flag to track if a confirmation is shown
    const idleMinutes = idleLimit / 60; // Convert idleLimit to minutes

    async function logout() {
        alert(`คุณถูกล็อกเอาต์เนื่องจากไม่ได้ใช้งานเป็นเวลา ${idleMinutes} นาที`);
        window.location.href = '/logout';
    }

    async function checkLogin() {
        try {
            const response = await fetch('/get-login-status');
            const data = await response.json();
            if (!data.loggedin) {
                window.location.href = '/login';
            }
        } catch (error) {
            console.error('Error checking login status:', error);
        }
    }

    function resetIdleTime() {
        idleTime = 0;
        confirmationShown = false; // Reset confirmation flag when user interacts
    }

    //checkLogin();
    setInterval(() => {
        idleTime++;
        if (idleTime >= idleLimit && !confirmationShown) {
            confirmationShown = true; // Set flag to indicate the confirmation is shown
            const confirmLogout = confirm(`คุณไม่ได้ใช้งานเป็นเวลา ${idleMinutes} นาที คุณต้องการออกจากระบบหรือไม่?`);

            if (confirmLogout) {
                logout();
            } else {
                idleTime = 0; // Reset idle time if user cancels
                confirmationShown = false; // Allow the confirmation to be shown again in the future
                checkLogin().then(() => {
                    console.log('Login status checked after cancellation');
                }).catch(error => {
                    console.error('Error checking login status after cancellation:', error);
                });
            }
        }
    }, 1000);

    window.onload = () => {
        document.addEventListener('mousemove', resetIdleTime);
        document.addEventListener('keypress', resetIdleTime);
        document.addEventListener('click', resetIdleTime);
        document.addEventListener('scroll', resetIdleTime);
    };
}

async function loadMenu() {
	try {
		const response = await fetch('/static/menu.html'); // Adjust the path if needed
		const menuHTML = await response.text();
		document.getElementById('menu-container').innerHTML = menuHTML;
	} catch (error) {
		console.error('Error loading menu:', error);
	}
}

function confirmLogout() {
	if (confirm('คุณแน่ใจหรือไม่ว่าจะออกจากระบบ?')) {
		clearCacheAndRedirectToLogout();
		return true;  // Proceed with logout
	}
	return false;  // Cancel logout
}

function clearCacheAndRedirectToLogout() {
	// Clear browser cache to prevent accessing pages after logout
	if (performance.getEntriesByType("navigation")[0].type === "back_forward") {
		// If accessed via back/forward, force a page reload
		window.location.reload();
	} else {
		// Perform logout and redirect to login page or dashboard
		window.location.href = '/logout';
	}
}

// Prevent accessing pages like the dashboard after logout
window.addEventListener('pageshow', function (event) {
	if (event.persisted || window.performance.getEntriesByType("navigation")[0].type === "back_forward") {
		window.location.href = '/login';  // Redirect to login if coming back after logout
	}
});

/////////////////////////////////////////////Arpo Above//////////////////////////////////////////////////

/**
 * Determines the maximum update speed
 * Requests to update data will happen no more frequently than
 * this time duration, in ms.
 */
var repeatMinWaitMS = 75;

/**
 * Determines when a request is considered "timed out".
 * The time out events listed below execute when this interval 
 * (in ms) elapses with no response.
 */
var timeOutMS = 5000;
 
/**
 * Allows the library to be disabled altogether
 */
var allowAJAX = true;

/**
 * Stores a queue of AJAX events to process
 */
var ajaxList = new Array();

/**
 * Initiates a new AJAX command
 *
 * @param   the url to access
 * @param   the document ID to fill with the result
 * @param	true to repeat this call, false for a single command
 */
function newAJAXCommand(url, container, repeat)
{
	if(!allowAJAX)
		return;
		
	//set up our object
	var newAjax = new Object();
	var theTimer = new Date();
	newAjax.url = url;
	newAjax.container = container;
	newAjax.repeat = repeat;
	newAjax.ajaxReq = null;
	
	//create and send the request
	if(window.XMLHttpRequest) {
        newAjax.ajaxReq = new XMLHttpRequest();
        newAjax.ajaxReq.open("POST", newAjax.url, true);
        newAjax.ajaxReq.send(null);
    //if we're using IE6 style (maybe 5.5 compatible too)
    } else if(window.ActiveXObject) {
        newAjax.ajaxReq = new ActiveXObject("Microsoft.XMLHTTP");
        if(newAjax.ajaxReq) {
            newAjax.ajaxReq.open("POST", newAjax.url, true);
            newAjax.ajaxReq.send();
        }
    }
    
    newAjax.lastCalled = theTimer.getTime();
    
    //store in our array
    ajaxList.push(newAjax);
}

/**
 * Loops over all pending AJAX events to determine
 * if any action is required
 */
function pollAJAX() {
	
	var curAjax = new Object();
	var theTimer = new Date();
	var elapsed;
	
	//read off the ajaxList objects one by one
	for(i = ajaxList.length; i > 0; i--)
	{
		curAjax = ajaxList.shift();
		if(!curAjax)
			continue;
		elapsed = theTimer.getTime() - curAjax.lastCalled;
				
		//if we suceeded
		if(curAjax.ajaxReq.readyState == 4 && curAjax.ajaxReq.status == 200) {
			//if it has a container, write the result
			if(curAjax.container){
				if(curAjax.container == 'status') {
					//we've defined 'status' as a special container
					document.getElementById('loading').style.display = 'none';
					document.getElementById('display').style.display = 'block';
					updateStatus(curAjax.ajaxReq.responseXML.documentElement);
				} else {
					//for all others, just write to the container
					document.getElementById(curAjax.container).innerHTML = curAjax.ajaxReq.responseText;
				}
			} //(otherwise do nothing)
			
	    	curAjax.ajaxReq.abort();
	    	curAjax.ajaxReq = null;

			//if its a repeatable request, then do so
			if(curAjax.repeat) {
				//try to wait at least repeatMinWaitMS so we don't flood the network
				if(elapsed > repeatMinWaitMS)
					newAJAXCommand(curAjax.url, curAjax.container, curAjax.repeat);
				else
					setTimeout("newAJAXCommand('"+curAjax.url+"', '"+curAjax.container+"', "+curAjax.repeat+")",repeatMinWaitMS-elapsed);
			}
			continue;
		}
		
		//if we've waited over 1 second, then we timed out
		if(elapsed > timeOutMS) {
			//notify the user
			if(curAjax.container == 'status') {
				//status is a special container
				document.getElementById('loading').style.display = 'block';
				document.getElementById('display').style.display = 'none';
			} else if(!curAjax.container) {
				//for command requests, alert the user
				alert("Command failed.\nConnection to demo board was lost.");
			}

	    	curAjax.ajaxReq.abort();
	    	curAjax.ajaxReq = null;
			
			//if its a repeatable request, then do so
			if(curAjax.repeat) {
				newAJAXCommand(curAjax.url, curAjax.container, curAjax.repeat);
			}
			
			continue;
		}
		
		//otherwise, just keep waiting
		ajaxList.push(curAjax);

	}//done cycling through all requests
	
	//call ourselves again in 10ms
	setTimeout("pollAJAX()",10);
	
}//end pollAjax
			
/**
 * Parses the xmlResponse returned by an XMLHTTPRequest object
 *
 * @param	the xmlData returned
 * @param	the field to search for
 */
function getXMLValue(xmlData, field) {
	try {
		if(xmlData.getElementsByTagName(field)[0].firstChild.nodeValue)
			return xmlData.getElementsByTagName(field)[0].firstChild.nodeValue;
		else
			return null;
	} catch(err) { return null; }
}

/**
 * Parses the xmlResponse from status.xml and updates the status box
 *
 * @param	the xmlData returned
 */
function updateStatus(xmlData) {
	
	//loop over all the LEDs
	for(i = 0; i < 8; i++) {
		if(getXMLValue(xmlData, 'led'+i) == '1')
			document.getElementById('led' + i).style.color = '#090';
		else
			document.getElementById('led' + i).style.color = '#ddd';
	}
	
	//loop over all the buttons
	for(i = 0; i < 4; i++) {
		if(getXMLValue(xmlData, 'btn'+i) == 'up')
			document.getElementById('btn' + i).innerHTML = '&Lambda;';
		else
			document.getElementById('btn' + i).innerHTML = 'V';
	}
	
	//update the POT value
	document.getElementById('pot0').innerHTML = getXMLValue(xmlData, 'pot0');
		
}

//kick off the AJAX Updater
setTimeout("pollAJAX()",500);

