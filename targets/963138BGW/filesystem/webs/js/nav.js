function side_nav(side_id){
	if (document.getElementById(side_id))
	{
		document.getElementById(side_id).className="linkselected";
	}

	else
		return;
}

function adv_nav(adv_id){
	/*
        if (adv_id.indexOf("broadbandsettings") == -1) {
            document.getElementById(adv_id).style.color = "#0FF";
            document.getElementById(adv_id).style.background = "none";
        }
        if ( adv_id.indexOf("advancedsetup") != -1 ) {
            side_id = adv_id.substring(14);

            switch (side_id) {
                case "servicesblocking":
                case "websiteblocking":
                case "schedulingaccess":
                    side_nav("servicesblocking"); break;

                case "broadbandsettings":
                case "atmsettings":
                case "ptmsettings":
                    side_nav("broadbandsettings"); break;

                case "dhcpsettings":
                case "lansubnets":
                case "lanipaddress":
                case "wanipaddress":
                case "vlan":
                    side_nav("dhcpsettings"); break;

                case "upstream":
                case "downstream":
                    side_nav("upstream"); break;

                case "remotetelnet":
                case "remotegui":
                    side_nav("remotegui"); break;

                case "dynamicrouting":
                case "staticrouting":
                    side_nav("dynamicrouting"); break;

                case "admin":
                case "advancedportforwarding":
                case "applications":
                case "dmzhosting":
                case "firewallsettings":
                case "nat":
                case "upnp":
                case "ipsec":
                    side_nav("admin"); break;
            }
        }
        else if ( adv_id.indexOf("advancedutilities") != -1 ) {
            side_nav("systemlog");
        }
	*/
}

function popUp(URL) {
  var day = new Date();
  var id  = day.getTime();
  var URL2 =  URL;
   eval("page" + id + " = window.open(URL2, '" + id + "', 'toolbar=0,scrollbars=1,location=0,statusbar=0,menubar=0,resizable=0,width=827,height=662');");
}

var clicked = 0;
var secLast = 0;
function goThere(where)
{
   var d = new Date();
   var secNow = Math.round(d/1000);
   if ((clicked == 0) || ((secNow - secLast) > 2))
   {
      secLast = secNow;
      clicked = 1;
	window.top.location.href=where;
      return true;
   }
   else
      return false;
}

//DOM-ready watcher
function domFunction(f, a)
{
	//initialise the counter
	var n = 0;

	//start the timer
	var t = setInterval(function()
	{
		//continue flag indicates whether to continue to the next iteration
		//assume that we are going unless specified otherwise
		var c = true;

		//increase the counter
		n++;

		//if DOM methods are supported, and the body element exists
		//(using a double-check including document.body, for the benefit of older moz builds [eg ns7.1]
		//in which getElementsByTagName('body')[0] is undefined, unless this script is in the body section)
		if(typeof document.getElementsByTagName != 'undefined' && (document.getElementsByTagName('body')[0] != null || document.body != null))
		{
			//set the continue flag to false
			//because other things being equal, we're not going to continue
			c = false;

			//but ... if the arguments object is there
			if(typeof a == 'object')
			{
				//iterate through the object
				for(var i in a)
				{
					//if its value is "id" and the element with the given ID doesn't exist
					//or its value is "tag" and the specified collection has no members
					if
					(
						(a[i] == 'id' && document.getElementById(i) == null)
						||
						(a[i] == 'tag' && document.getElementsByTagName(i).length < 1)
					)
					{
						//set the continue flag back to true
						//because a specific element or collection doesn't exist
						c = true;

						//no need to finish this loop
						break;
					}
				}
			}

			//if we're not continuing
			//we can call the argument function and clear the timer
			if(!c) { f(); clearInterval(t); }
		}

		//if the timer has reached 60 (so timeout after 15 seconds)
		//in practise, I've never seen this take longer than 7 iterations [in kde 3
		//in second place was IE6, which takes 2 or 3 iterations roughly 5% of the time]
		if(n >= 60)
		{
			//clear the timer
			clearInterval(t);
		}

	}, 20);
};
