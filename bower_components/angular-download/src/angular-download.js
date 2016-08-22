"use strict";
var downloadLink;
angular.module("download", [])
	.run(function() {
		var template = '<angular-download style="display: none"><a></a></angular-download>';
		downloadLink = angular.element(document.body).append(template).find('angular-download').find("a");
	})
	.factory("download", function() {

		return {
			fromData: function(data, mimeType, name) {
				this.fromDataURL("data:" + mimeType + ";base64," + btoa(data), name);
			},
			fromDataURL: function(dataUrl, name) {
				downloadLink.attr("href", dataUrl);
				downloadLink.attr("download", name || "");
				downloadLink[0].click();
			}
		};
	});