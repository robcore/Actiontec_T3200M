// JavaScript Document
// This function is important! It is used to solve the cache problem for upgrade from sdk3 to sdk12.
function ifLoadSuccess(){
	return true;
}
//Popup function
function popUp(URL) {
	day = new Date();
	id = day.getTime();
	eval("page" + id + " = window.open(URL, '" + id + "', 'toolbar=0,scrollbars=1,location=0,statusbar=0,menubar=0,resizable=0,width=827,height=662');");
}

function appendTableRow(table, align, content){
    var tr = table.insertRow(-1);
    var i;
    for(i=0; i < content.length; i++){
        var td = tr.insertCell(-1);
        td.align = align;
        td.innerHTML = content[i];
    }
}

function insertTableRow(table, index, align, content){
    var tr = table.insertRow(index);
    var i;
    for(i=0; i < content.length; i++){
        var td = tr.insertCell(-1);
        td.align = align;
        td.innerHTML = content[i];
    }
}
//show/hide steps
function showHideSteps(hideThis, showThis){
	var hideThis = document.getElementById(hideThis);
	var showThis =document.getElementById(showThis);
	hideThis.style.display ='none';
	showThis.style.display ='block';
}
var vis=1;
function cover(me) {
	var cover=document.getElementById(me);
	if(vis){
		vis=0;
		cover.style.display='block';
    }
	else
	{
		vis=1;
		cover.style.display='none';
    }
}

function appendTableEmptyRow(table, align, colspan, content){
    var tr = table.insertRow(-1);
    var td = tr.insertCell(-1);
    if (isNaN(colspan))
        colspan = 1;
    td.align = align;
    td.colSpan = colspan;
    td.innerHTML = content;
}

function get_defaultKeyFlagArray(flagArray, flag)
{
    for (var i=0; i<4; i++)
	flagArray[i] = flag >> i & 1;
}

function set_defaultKeyFlag(flag, idx, sts)
{
    var ret;
    if (sts == "1")
    ret = flag | 1 << idx;
    else {
	var temp = 1;
	var sum = 0;
	for (var i=0; i<4; i++) {
	    if (idx != i)
		sum += temp;
	    temp = temp * 2;
	}
	ret = flag & sum;
    }
    return ret;
}

/* modify thd "idx" bit of "sStr" to "bit" */
function modifyBit(sStr, idx, bit)
{
    var idx_num=parseInt(idx, 10);
    var from_pat = new RegExp("(.{" + idx_num + "}).");
    var to_pat = "$1" + bit;
    return sStr.replace(from_pat, to_pat);
}

//for change submit method from Get to Post
function postSubmit(lochead){

	var action = lochead.substr(0,lochead.indexOf("?"));
	var paramStr =  lochead.substr(lochead.indexOf("?")+1,lochead.length);
	var paramArray = paramStr.split("&");
	var tempForm = document.createElement("form");
	tempForm.name = "postForm";
	tempForm.action = action;
	tempForm.method = "post";
	document.body.appendChild(tempForm);
	if(paramArray.length>0){
		for(var i=0;i<paramArray.length;i++){
			var paramArrayValue = paramArray[i];
			//deal with "&" which in the value
			if(paramArrayValue){
				var regeFir = /\%260.6117115231341502*/;
				var matFir = paramArrayValue.match(regeFir);
				if (matFir)
				{
					var reFir = /\%260.6117115231341502/g;
					paramArrayValue = paramArrayValue.replace(reFir, "\&");
				}
			}
			//paramArrayValue = unescape(paramArrayValue)
			if(paramArrayValue!=""){
				var param = paramArrayValue.split("=");
				var paramName = param[0];
				var paramValue = paramArrayValue.substr((paramName.length)+1,paramArrayValue.length);
				var tempInput = document.createElement("input");
				tempInput.type="hidden";
				tempInput.name=paramName;
				tempInput.value=paramValue;
				tempForm.appendChild(tempInput);
			}
		}
	}
	tempForm.submit();
}
//change submit method from Get to Post for thankyou pages
function postSubmitThankyou(lochead){
	var action = lochead.substr(0,lochead.indexOf("?"));
	var paramStr =  lochead.substr(lochead.indexOf("?")+1,lochead.length);
	var paramArray = paramStr.split("&");
	var tempForm = document.createElement("form");
	tempForm.name = "postForm";
	tempForm.action = action;
	tempForm.method = "post";
	document.body.appendChild(tempForm);
	if(paramArray.length>0){
		for(var i=0;i<paramArray.length;i++){
			var paramArrayValue = paramArray[i];
			paramArrayValue = unescape(paramArrayValue);
			if(paramArrayValue!=""){
				var param = paramArrayValue.split("=");
				var paramName = param[0];
				var paramValue = paramArrayValue.substr((paramName.length)+1,paramArrayValue.length);
				var tempInput = document.createElement("input");
				tempInput.type="hidden";
				tempInput.name=paramName;
				tempInput.value=paramValue;
				tempForm.appendChild(tempInput);
			}
		}
	}
	tempForm.submit();
}
//Base 64 encode replace window.btoa.
var _keyStr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
function base64_btoa(input){
    if(input == "")
        return input;
    var output = "";
    var chr1, chr2, chr3, enc1, enc2, enc3, enc4;
    var i = 0;
    while(i < input.length)
    {
        chr1 = input.charCodeAt(i++);
        chr2 = input.charCodeAt(i++);
        chr3 = input.charCodeAt(i++);
        enc1 = chr1 >> 2;
        enc2 = ((chr1 & 3) << 4) | (chr2 >> 4);
        enc3 = ((chr2 & 15) << 2) | (chr3 >> 6);
        enc4 = chr3 & 63;
        if(isNaN(chr2))
        {
            enc3 = 64;
            enc4 = 64;
        }
        else if(isNaN(chr3))
        {
            enc4 = 64;
        }
        output = output +_keyStr.charAt(enc1) + _keyStr.charAt(enc2) + _keyStr.charAt(enc3) + _keyStr.charAt(enc4);
    }
    return output;
}

var xmlhttp = null;
//function createXmlhttp()
//{
	if(window.XMLHttpRequest)  //Mozilla
	{
		xmlhttp = new XMLHttpRequest();
		/*if (xmlhttp.overrideMimeType)
		{
			xmlhttp.overrideMimeType("text/xml");
		}*/

	}
	else if(window.ActiveXObject) //IE
	{
		try
		{
			xmlhttp = new ActiveXObject("Msxml2.XMLHTTP"); //new IE
		}
		catch(e)
		{
			xmlhttp = new ActiveXObject("Microsoft.XMLHTTP"); //old IE
		}
	}
	if(!xmlhttp)
	{
		window.alert("_(Your broswer not support XMLHttpRequest!)_");
	}
//}
