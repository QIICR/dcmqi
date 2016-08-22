# angular-download

Service for downloading browser-generated data to disk.

Motivation
----------

This service provides methods for downloading content generated in JavaScript (text, images, etc.) as files.
It relies on the `<a download>` functionality supported in current browsers and does not require any server
handling or flash plugins.

Installing the Module
---------------------
Installation can be done through bower:
``` shell
bower install angular-download
```

In your page add:
```html
  <script src="bower_components/angular-download/angular-download.js"></script>
```

You can also use npm to install it:

``` shell
npm install angular-download --save
```

Loading the Module
------------------

This service is part of the `download` module:

```javascript
var app = angular.module('myApp', [..., 'download']);
```

Using the download service
--------------------------

Inject the `download` service:

```javascript
function MyController($scope, download, ...) {
	$scope.downloadFile = function() {
		download.fromData("contents of the file", "text/plain", "file.txt");
	}
}
```

Method: fromData(data, mimeType, fileName)
------------------------------------------

Downloads a file containing `data` as type `mimeType` named `fileName`.

```javascript
function MyController($scope, download) {
	$scope.downloadAsJson = function(someData) {
		download.fromData(JSON.stringify(someData), "application/json", "download.json");
	}
}
```

Method: fromDataURL(dataUrl, fileName)
--------------------------------------

Downloads a file with contents defined by the `dataUrl` named `fileName`.  This is useful for downloading binary
data, such as client-generated images.

```javascript
function MyController($scope, download) {
	$scope.downloadImage = function(img) {
		download.fromDataURL(getImageDataURL(img), "download.png");
	}
}

// Create a dataURL from an img element
function getImageDataURL(img) {
	// Create an empty canvas element
	var canvas = document.createElement("canvas");

	// Copy the image contents to the canvas
	canvas.width = img.width;
	canvas.height = img.height;
	var ctx = canvas.getContext("2d");
	ctx.drawImage(img, 0, 0);

	return canvas.toDataURL("image/png");
}
```

Copyright & License
-------------------

Copyright 2015 Stepan Riha. All Rights Reserved.

This may be redistributed under the MIT licence. For the full license terms, see the LICENSE file which
should be alongside this readme.

