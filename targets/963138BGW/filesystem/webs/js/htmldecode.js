//Decode the single variable
function htmlDecodeStr(str) {
	if(str == undefined)
		return "";
	if (str.search("&#") == -1)
		return str;

	var re = /&#[1-9][0-9]{0,2};/g;
	var s_des = str.replace(re, function($0){
									var s_tmp = $0;
									var r0 = /&#/g;
									s_tmp = s_tmp.replace(r0, "");
									var r1 = /;/g;
									s_tmp = s_tmp.replace(r0, "");
									s_tmp = String.fromCharCode(parseInt(s_tmp));
								return(s_tmp)});

	return s_des;
}

// Decode all input(hidden) widgets
function htmlDecode() {
	for (var n=0; n<(document.forms.length); n++)
	{
		var form = document.forms[n];
		for (var i = 0; i < form.elements.length; i++) {
			if ((form.elements[i].type == "text" || form.elements[i].type == "hidden") && form.elements[i].value != "")
			form.elements[i].value = htmlDecodeStr(form.elements[i].value);
		}
	}
}

//check if has microsoft special characters<>"'%;()&+
function hasSpecialChar(str)
{
	var re = /^[^<>()"'%&+;]+$/;
	return !re.test(str);
}

//check if has html tags
function hasHTMLTags(str){
	var ifHasHTMLTgs= false;
	var htmlTagsArray = ["<applet","<input","<body","<embed","<frame","<script","<javascript","<img","<iframe","<style","<link","<ilayer","<meta","<object","</script","<tr","<td","<th","</tr","</td","</th","<table","</table","<form","</form"];
	for(var i=0;i<htmlTagsArray.length;i++){
		if(str.indexOf(htmlTagsArray[i])!=-1){
			ifHasHTMLTgs = true;
			break;
		}
	}
	return ifHasHTMLTgs;
}

//Encode the variable(May be used in the table view)
function htmlEncodeStr(str)
{
	var re = /<|>|"|'|%|;|\(|\)|&|\+/gi;
	var s_des = str.replace(re, function($0){
								var i_tmp = $0.charCodeAt();
								return("&#" + i_tmp + ";")});
	return s_des;
}
