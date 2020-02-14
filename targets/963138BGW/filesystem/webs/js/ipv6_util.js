
function isipv6(ip)
{
	var perlipv6regex = "^\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:)))(%.+)?\s*$";
	var aeronipv6regex = "^\s*((?=.*::.*)(::)?([0-9A-F]{1,4}(:(?=[0-9A-F])|(?!\2)(?!\5)(::)|\z)){0,7}|((?=.*::.*)(::)?([0-9A-F]{1,4}(:(?=[0-9A-F])|(?!\7)(?!\10)(::))){0,5}|([0-9A-F]{1,4}:){6})((25[0-5]|(2[0-4]|1[0-9]|[1-9]?)[0-9])(\.(?=.)|\z)){4}|([0-9A-F]{1,4}:){7}[0-9A-F]{1,4})\s*$";

	var regex = "/"+perlipv6regex+"/";
	return (/^\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:)))(%.+)?\s*$/.test(ip));
}

function isLLA(ip)
{
	var s = ip.substr(0, 4).toLowerCase();
	if (s == "fe80")
		return true;
	else
		return false;
}

function isValidIPv6AddrLen(len)
{
	if (len.length < 1 || isNaN(len) || parseInt(len) > 128 || parseInt(len) < 16 /*|| parseInt(len)%8!=0*/)
	{
		return false;
	}
	return true;
}

function isValidIPv6PrefixLen(len)
{
	if (len.length < 1 || isNaN(len) || parseInt(len) > 64 || parseInt(len) < 16 /*|| parseInt(len)%8!=0*/)
	{
		return false;
	}
	return true;
}

function isValidIPv4Len(len)
{
	if (len.length < 1 || isNaN(len) || parseInt(len) > 32 || parseInt(len) < 0 /*|| parseInt(len)%8!=0*/)
	{
		return false;
	}
	return true;
}

function switchIPv6Address(address)
{
	if(address.indexOf("/") != -1) {
		address = address.split("/")[0];
	}

	var index = address.indexOf("::");
	if(index != -1) {
		var pre_address = address.substring(0, index);
		var suf_address = address.substring(index+2, address.length);

		var pre_colon = 0;
		var suf_colon = 0;
		if(pre_address.indexOf(":") != -1) {
			pre_colon = pre_address.split(":").length - 1;
		}
		if(suf_address.indexOf(":") != -1) {
			suf_colon = suf_address.split(":").length - 1;
		}
		var added_colon = 6 - pre_colon - suf_colon;
		for(var i = 0; i<added_colon; i++) {
			pre_address += ":0";
		}
		pre_address += ":";
		return pre_address + suf_address;
	} else {
		return address;
	}
}


function right_switchIPv6Address(address)
{
	var addr_suf="";
	var addr_arry=new Array();
	   for(var i=0;i<8;i++)
	   {
		addr_arry[i]=address.split(":")[i];
		if(addr_arry[i]=="0000"||addr_arry[i]=="000"||addr_arry[i]=="00")
		    addr_arry[i]="0";
		if(addr_arry[i].length==2)
		  {
			if(addr_arry[i].substring(0,1)=="0")
				addr_arry[i]=addr_arry[i].substring(1,2);
		  }
		if(addr_arry[i].length==3)
		  {
			if(addr_arry[i].substring(0,1)=="0")
				{
				addr_arry[i]=addr_arry[i].substring(1,3);
			     if(addr_arry[i].substring(0,1)=="0")
				addr_arry[i]=addr_arry[i].substring(1,2);
				}
		  }
		if(addr_arry[i].length==4)
		  {
			if(addr_arry[i].substring(0,1)=="0")
				{
				addr_arry[i]=addr_arry[i].substring(1,4);
			         if(addr_arry[i].substring(0,1)=="0")
				  {
				   addr_arry[i]=addr_arry[i].substring(1,3);
				      if(addr_arry[i].substring(0,1)=="0")
						addr_arry[i]=addr_arry[i].substring(1,2);
				  }
				}
		  }
	   }

	   for(var j=1;j<8;j++)
	   {
		 addr_suf+=":"+addr_arry[j];
	   }

	   addr_suf=addr_arry[0]+addr_suf;
	   //alert("suf="+addr_suf+"@@len="+addr_suf.length);
		return addr_suf;

}



function cutIPv6Address(address)
{
	var length = 0;
	var cutAddress = "";
	if(address.indexOf("/") != -1) {
		length = address.split("/")[1];
	}
	var size = length / 16;
	address = switchIPv6Address(address);

	var address_array = address.split(":");
	for(var i = 0; i < size; i++) {
		cutAddress += address_array[i] + ":";
	}
	return cutAddress.substring(0, cutAddress.length - 1);
}

function checkBitIPv6Address(address)
{
	var index = address.indexOf("::");
	if(index != -1) {
		address = address.substring(0, index);
	}
	return getAddressBit(address);
}

function getAddressBit(address)
{
	var address_array = address.split(":");
	var address_2 = "";
	var address_sum = "";
	var size = 0;
	for(var i = 0; i < address_array.length; i++)
	{
		if(address_array[i] != "") {
			address_2 = parseInt(address_array[i], 16).toString(2);
			if(address_2.length < 16) {
				var length = 16 - address_2.length;

				for(var j = 0; j < length; j++) {
					address_2 = "0" + address_2;
				}
			}
			address_sum += address_2;
		}
	}
	return address_sum.lastIndexOf("1") + 1;
}

function splitZeroPrefix(str)
{
	 var re = /0*(\d+)/;
	 str = switchCase(str);
	 return str.replace(re, "$1");
}

function switchCase(str)
{
	return str.toLowerCase();
}

function pop_up(str, flag, callback)
{
	if(flag == 0 || flag == "0")
	{
		alert(str);
	}
	else if(flag == 1 || flag == "1")
	{
		if(confirm(str))
		{
			callback();
		}
	}
}
