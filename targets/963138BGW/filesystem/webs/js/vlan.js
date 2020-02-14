function side_nav(side_id){
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

function isHexaDigit(digit) {
   var hexVals = new Array("0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                           "A", "B", "C", "D", "E", "F", "a", "b", "c", "d", "e", "f");
   var len = hexVals.length;
   var i = 0;
   var ret = false;

   for ( i = 0; i < len; i++ )
      if ( digit == hexVals[i] ) break;

   if ( i < len )
      ret = true;

   return ret;
}

function isValidKey(val, size) {
   var ret = false;
   var len = val.length;
   var dbSize = size * 2;

   if ( len == size )
      ret = true;
   else if ( len == dbSize ) {
      for ( i = 0; i < dbSize; i++ )
         if ( isHexaDigit(val.charAt(i)) == false )
            break;
      if ( i == dbSize )
         ret = true;
   } else
      ret = false;

   return ret;
}


function isValidHexKey(val, size) {
   var ret = false;
   if (val.length == size) {
      for ( i = 0; i < val.length; i++ ) {
         if ( isHexaDigit(val.charAt(i)) == false ) {
            break;
         }
      }
      if ( i == val.length ) {
         ret = true;
      }
   }

   return ret;
}


function isNameUnsafe(compareChar) {
   var unsafeString = "\"<>%\\^[]`\+\$\,='#&@.: \t";

   if ( unsafeString.indexOf(compareChar) == -1 && compareChar.charCodeAt(0) > 32
        && compareChar.charCodeAt(0) < 123 )
      return false; // found no unsafe chars, return false
   else
      return true;
}

// Check if a name valid
function isValidName(name) {
   var i = 0;

   for ( i = 0; i < name.length; i++ ) {
      if ( isNameUnsafe(name.charAt(i)) == true )
         return false;
   }

   return true;
}

// same as is isNameUnsafe but allow spaces
function isCharUnsafe(compareChar) {
   var unsafeString = "\"<>%\\^[]`\+\$\,='#&@.:\t";

   if ( unsafeString.indexOf(compareChar) == -1 && compareChar.charCodeAt(0) >= 32
        && compareChar.charCodeAt(0) < 123 )
      return false; // found no unsafe chars, return false
   else
      return true;
}

function isValidNameWSpace(name) {
   var i = 0;

   for ( i = 0; i < name.length; i++ ) {
      if ( isCharUnsafe(name.charAt(i)) == true )
         return false;
   }

   return true;
}

// The alias must be alphanumeric
function isAliasUnsafe(aliasChar) {
   var unsafeString = ":;<=>?@[\\]^_`";

   if ( unsafeString.indexOf(aliasChar) == -1 &&
        aliasChar.charCodeAt(0) >= 48 &&
        aliasChar.charCodeAt(0) < 123 )
     return false; //found no unsafe chars, return false
   if (aliasChar == ' ')
       return false;
   return true;
}

function  isValidAlias(alias) {
   var i = 0;

   for ( i = 0; i < alias.length; i++ ) {
      if ( isAliasUnsafe(alias.charAt(i)) == true )
         return false;
   }

   return true;
}

function isSameSubNet(lan1Ip, lan1Mask, lan2Ip, lan2Mask) {

   var count = 0;

   lan1a = lan1Ip.split('.');
   lan1m = lan1Mask.split('.');
   lan2a = lan2Ip.split('.');
   lan2m = lan2Mask.split('.');

   for (i = 0; i < 4; i++) {
      l1a_n = parseInt(lan1a[i]);
      l1m_n = parseInt(lan1m[i]);
      l2a_n = parseInt(lan2a[i]);
      l2m_n = parseInt(lan2m[i]);
      if ((l1a_n & l1m_n) == (l2a_n & l2m_n))
         count++;
   }
   if (count == 4)
      return true;
   else
      return false;
}


function isValidIpAddress(address) {
   var i = 0;
   var ipPattern = /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/;
   var ipArray = address.match(ipPattern);

   if (ipArray == null)
      return false;


   if ( address == '0.0.0.0' ||
        address == '255.255.255.255' )
      return false;

   addrParts = address.split('.');
   if ( addrParts.length != 4 ) return false;
   for (i = 0; i < 4; i++) {
      if (isNaN(addrParts[i]) || addrParts[i] =="")
         return false;
      num = parseInt(addrParts[i]);
      if ( num < 0 || num > 255 )
         return false;
   }
   return true;
}

function isValidIpAddress6(address) {
   var i = 0, num = 0;

   addrParts = address.split(':');
   if (addrParts.length < 3 || addrParts.length > 8)
      return false;
   for (i = 0; i < addrParts.length; i++) {
      if ( addrParts[i] != "" )
         num = parseInt(addrParts[i], 16);
      if ( i == 0 ) {
//         if ( (num & 0xf000) == 0xf000 )
//            return false;	//can not be link-local, site-local or multicast address
      }
      else if ( (i + 1) == addrParts.length) {
         if ( num == 0 || num == 1)
            return false;	//can not be unspecified or loopback address
      }
      if ( num != 0 )
         break;
   }
   return true;
}

function isValidPrefixLength(prefixLen) {
   var num;

   num = parseInt(prefixLen);
   if (num <= 0 || num > 128)
      return false;
   return true;
}

function areSamePrefix(addr1, addr2) {
   var i, j;
   var a = [0, 0, 0, 0, 0, 0, 0, 0];
   var b = [0, 0, 0, 0, 0, 0, 0, 0];

   addr1Parts = addr1.split(':');
   if (addr1Parts.length < 3 || addr1Parts.length > 8)
      return false;
   addr2Parts = addr2.split(':');
   if (addr2Parts.length < 3 || addr2Parts.length > 8)
      return false;
   j = 0;
   for (i = 0; i < addr1Parts.length; i++) {
      if ( addr1Parts[i] == "" ) {
		 if ((i != 0) && (i+1 != addr1Parts.length)) {
			j = j + (8 - addr1Parts.length + 1);
		 }
		 else {
		    j++;
		 }
	  }
	  else {
         a[j] = parseInt(addr1Parts[i], 16);
		 j++;
	  }
   }
   j = 0;
   for (i = 0; i < addr2Parts.length; i++) {
      if ( addr2Parts[i] == "" ) {
		 if ((i != 0) && (i+1 != addr2Parts.length)) {
			j = j + (8 - addr2Parts.length + 1);
		 }
		 else {
		    j++;
		 }
	  }
	  else {
         b[j] = parseInt(addr2Parts[i], 16);
		 j++;
	  }
   }
   //only compare 64 bit prefix
   for (i = 0; i < 4; i++) {
      if (a[i] != b[i]) {
	     return false;
	  }
   }
   return true;
}

function getLeftMostZeroBitPos(num) {
   var i = 0;
   var numArr = [128, 64, 32, 16, 8, 4, 2, 1];

   for ( i = 0; i < numArr.length; i++ )
      if ( (num & numArr[i]) == 0 )
         return i;

   return numArr.length;
}

function getRightMostOneBitPos(num) {
   var i = 0;
   var numArr = [1, 2, 4, 8, 16, 32, 64, 128];

   for ( i = 0; i < numArr.length; i++ )
      if ( ((num & numArr[i]) >> i) == 1 )
         return (numArr.length - i - 1);

   return -1;
}

function isValidSubnetMask(mask) {
   var i = 0, num = 0;
   var zeroBitPos = 0, oneBitPos = 0;
   var zeroBitExisted = false;

   if ( mask == '0.0.0.0' )
      return false;

   maskParts = mask.split('.');
   if ( maskParts.length != 4 ) return false;

   for (i = 0; i < 4; i++) {
      if ( isNaN(maskParts[i]) == true )
         return false;
      num = parseInt(maskParts[i]);
      if ( num < 0 || num > 255 )
         return false;
      if ( zeroBitExisted == true && num != 0 )
         return false;
      zeroBitPos = getLeftMostZeroBitPos(num);
      oneBitPos = getRightMostOneBitPos(num);
      if ( zeroBitPos < oneBitPos )
         return false;
      if ( zeroBitPos < 8 )
         zeroBitExisted = true;
   }

   return true;
}

function isValidPort(port) {
   var fromport = 0;
   var toport = 100;

   portrange = port.split(':');
   if ( portrange.length < 1 || portrange.length > 2 ) {
       return false;
   }
   if ( isNaN(portrange[0]) )
       return false;
   fromport = parseInt(portrange[0]);
   if ( isNaN(fromport) )
       return false;

   if ( portrange.length > 1 ) {
       if ( isNaN(portrange[1]) )
          return false;
       toport = parseInt(portrange[1]);
       if ( isNaN(toport) )
           return false;
       if ( toport <= fromport )
           return false;
   }

   if ( fromport < 1 || fromport > 65535 || toport < 1 || toport > 65535 )
       return false;

   return true;
}

function isValidNatPort(port) {
   var fromport = 0;
   var toport = 100;

   portrange = port.split('-');
   if ( portrange.length < 1 || portrange.length > 2 ) {
       return false;
   }
   if ( isNaN(portrange[0]) )
       return false;
   fromport = parseInt(portrange[0]);

   if ( portrange.length > 1 ) {
       if ( isNaN(portrange[1]) )
          return false;
       toport = parseInt(portrange[1]);
       if ( toport <= fromport )
           return false;
   }

   if ( fromport < 1 || fromport > 65535 || toport < 1 || toport > 65535 )
       return false;

   return true;
}

function isValidMacAddress(address) {
   var c = '';
   var num = 0;
   var i = 0, j = 0;
   var zeros = 0;

   addrParts = address.split(':');
   if ( addrParts.length != 6 ) return false;

   for (i = 0; i < 6; i++) {
      if ( addrParts[i] == '' )
         return false;
      for ( j = 0; j < addrParts[i].length; j++ ) {
         c = addrParts[i].toLowerCase().charAt(j);
         if ( (c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') )
            continue;
         else
            return false;
      }

      num = parseInt(addrParts[i], 16);
      if ( num == NaN || num < 0 || num > 255 )
         return false;
      if ( num == 0 )
         zeros++;
   }
   if (zeros == 6)
      return false;

   return true;
}

function isValidMacMask(mask) {
   var c = '';
   var num = 0;
   var i = 0, j = 0;
   var zeros = 0;
   var zeroBitPos = 0, oneBitPos = 0;
   var zeroBitExisted = false;

   maskParts = mask.split(':');
   if ( maskParts.length != 6 ) return false;

   for (i = 0; i < 6; i++) {
      if ( maskParts[i] == '' )
         return false;
      for ( j = 0; j < maskParts[i].length; j++ ) {
         c = maskParts[i].toLowerCase().charAt(j);
         if ( (c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') )
            continue;
         else
            return false;
      }

      num = parseInt(maskParts[i], 16);
      if ( num == NaN || num < 0 || num > 255 )
         return false;
      if ( zeroBitExisted == true && num != 0 )
         return false;
      if ( num == 0 )
         zeros++;
      zeroBitPos = getLeftMostZeroBitPos(num);
      oneBitPos = getRightMostOneBitPos(num);
      if ( zeroBitPos < oneBitPos )
         return false;
      if ( zeroBitPos < 8 )
         zeroBitExisted = true;
   }
   if (zeros == 6)
      return false;

   return true;
}

var hexVals = new Array("0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
              "A", "B", "C", "D", "E", "F");
var unsafeString = "\"<>%\\^[]`\+\$\,'#&";
// deleted these chars from the include list ";", "/", "?", ":", "@", "=", "&" and #
// so that we could analyze actual URLs

function isUnsafe(compareChar)
// this function checks to see if a char is URL unsafe.
// Returns bool result. True = unsafe, False = safe
{
   if ( unsafeString.indexOf(compareChar) == -1 && compareChar.charCodeAt(0) > 32
        && compareChar.charCodeAt(0) < 123 )
      return false; // found no unsafe chars, return false
   else
      return true;
}

function decToHex(num, radix)
// part of the hex-ifying functionality
{
   var hexString = "";
   while ( num >= radix ) {
      temp = num % radix;
      num = Math.floor(num / radix);
      hexString += hexVals[temp];
   }
   hexString += hexVals[num];
   return reversal(hexString);
}

function reversal(s)
// part of the hex-ifying functionality
{
   var len = s.length;
   var trans = "";
   for (i = 0; i < len; i++)
      trans = trans + s.substring(len-i-1, len-i);
   s = trans;
   return s;
}

function convert(val)
// this converts a given char to url hex form
{
   return  "%" + decToHex(val.charCodeAt(0), 16);
}


function encodeUrl(val)
{
   var len     = val.length;
   var i       = 0;
   var newStr  = "";
   var original = val;

   for ( i = 0; i < len; i++ ) {
      if ( val.substring(i,i+1).charCodeAt(0) < 255 ) {
         // hack to eliminate the rest of unicode from this
         if (isUnsafe(val.substring(i,i+1)) == false)
            newStr = newStr + val.substring(i,i+1);
         else
            newStr = newStr + convert(val.substring(i,i+1));
      } else {
         // woopsie! restore.
         alert ("Found a non-ISO-8859-1 character at position: " + (i+1) + ",\nPlease eliminate before continuing.");
         newStr = original;
         // short-circuit the loop and exit
         i = len;
      }
   }

   return newStr;
}

var markStrChars = "\"'";

// Checks to see if a char is used to mark begining and ending of string.
// Returns bool result. True = special, False = not special
function isMarkStrChar(compareChar)
{
   if ( markStrChars.indexOf(compareChar) == -1 )
      return false; // found no marked string chars, return false
   else
      return true;
}

// use backslash in front one of the escape codes to process
// marked string characters.
// Returns new process string
function processMarkStrChars(str) {
   var i = 0;
   var retStr = '';

   for ( i = 0; i < str.length; i++ ) {
      if ( isMarkStrChar(str.charAt(i)) == true )
         retStr += '\\';
      retStr += str.charAt(i);
   }

   return retStr;
}

// Web page manipulation functions

function showhide(element, sh)
{
    var status;
    if (sh == 1) {
        status = "block";
    }
    else {
        status = "none"
    }

	if (document.getElementById)
	{
		// standard
		document.getElementById(element).style.display = status;
	}
	else if (document.all)
	{
		// old IE
		document.all[element].style.display = status;
	}
	else if (document.layers)
	{
		// Netscape 4
		document.layers[element].display = status;
	}
}

// Load / submit functions

function getSelect(item)
{
	var idx;
	if (item.options.length > 0) {
	    idx = item.selectedIndex;
	    return item.options[idx].value;
	}
	else {
		return '';
    }
}

function setSelect(item, value)
{
	for (i=0; i<item.options.length; i++) {
        if (item.options[i].value == value) {
		item.selectedIndex = i;
		break;
        }
    }
}

function setCheck(item, value)
{
    if ( value == '1' ) {
         item.checked = true;
    } else {
         item.checked = false;
    }
}

function setDisable(item, value)
{
    if ( value == 1 || value == '1' ) {
         item.disabled = true;
    } else {
         item.disabled = false;
    }
}

function submitText(item)
{
	return '&' + item.name + '=' + item.value;
}

function submitSelect(item)
{
	return '&' + item.name + '=' + getSelect(item);
}


function submitCheck(item)
{
	var val;
	if (item.checked == true) {
		val = 1;
	}
	else {
		val = 0;
	}
	return '&' + item.name + '=' + val;
}

/*
 *  Input   addr:       addr is a string, which is net address such as '192.168.10.5'
 *          bit_val:    bit_val is an array
 *                      length is 32
 *                      countains the binary NetMask
 */
function changeNetAddrIntoBit(addr, bit_val)
{
    var val = addr.split('.');
    var count = 0;
    for (j = 0; j < 4; j++)
    {
        var bit8_val = parseInt(val[3-j]);
        for (i = 0; i < 8; i ++)
        {
            bit_val[31-count] = bit8_val%2;
            if (bit_val[31-count] == 0)
                bit8_val = bit8_val/2;
            else
                bit8_val = (bit8_val -1)/2;
            count++;
        }
    }
}

/*
 *  Input:  Mask_bit   Mask_bit is an array
 *                     length is 32
 *                     countains the binary NetMask
 */
function getMaskBitOneNum(Mask_bit)
{
    for (i = 0; i<32; i++)
    {
        if ( Mask_bit[i] == 0)
            return i;
    }
    return 32;
}

/*
 *  Return: 1       lan1 is a subnet of lan2
 *          0       lan1 is the same as lan2
 *          -1      lan2 is a subnet of lan1
 *          100    lan1 and lan2 don't have any relationship
 *
 */
function compareSubNet(lan1Ip, lan1Mask, lan2Ip, lan2Mask)
{
    var lan1Ip_bit = new Array();
    var lan1Mask_bit = new Array();
    var lan2Ip_bit = new Array();
    var lan2Mask_bit = new Array();
    changeNetAddrIntoBit(lan1Ip, lan1Ip_bit);
    changeNetAddrIntoBit(lan1Mask, lan1Mask_bit);
    changeNetAddrIntoBit(lan2Ip, lan2Ip_bit);
    changeNetAddrIntoBit(lan2Mask, lan2Mask_bit);
    var lan1_Mask_len = getMaskBitOneNum(lan1Mask_bit);
    var lan2_Mask_len = getMaskBitOneNum(lan2Mask_bit);
    if (lan1_Mask_len < lan2_Mask_len)
    {
        for (i = 0; i<lan1_Mask_len; i++)
        {
            if (lan1Ip_bit[i] != lan2Ip_bit[i])
                return 100;
        }
        return -1;
    }
    else if(lan1_Mask_len > lan2_Mask_len)
    {
        for (i = 0; i<lan2_Mask_len; i++)
        {
            if (lan1Ip_bit[i] != lan2Ip_bit[i])
                return 100;
        }
        return 1;
    }
    else
    {
        for (i = 0; i<lan1_Mask_len; i++)
        {
            if (lan1Ip_bit[i] != lan2Ip_bit[i])
                return 100;
        }
        return 0;

    }
}

/*
 *  Input   IpAddr: ip's address such as "192.168.0.1"
 *  Return  1:  the ip is in the range: 0~127.255.255.255
 *          2:  the ip is in the range: 128~192.255.255.255
 *          3:  the ip is in the range: 193~238.255.255.255
 *          4:  the ip is in the range: 225~240.255.255.255  muticast ip Address
 *          5:  the ip is in the range: 240~255.255.255.255  broadcast ip Address
 */
function checkIPType(IpAddr)
{
    var val = IpAddr.split('.');
    var byte1_val = parseInt(val[0]);
    if (byte1_val < 128)
        return 1;
    else if (byte1_val < 192)
        return 2;
    else if (byte1_val < 224)
        return 3;
    else if (byte1_val < 240)
        return 4;
    else
        return 5;
}

// JavaScript Document
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
