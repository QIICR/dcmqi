require.config({
  paths: {
    "angular": "../bower_components/angular/angular",
    "jquery": "../bower_components/jquery/dist/jquery.min",
    "angular-animate" : "../bower_components/angular-animate/angular-animate",
    "v-accordion": "../bower_components/v-accordion/dist/v-accordion",
    "angular-aria": "../bower_components/angular-aria/angular-aria",
    "angular-messages": "../bower_components/angular-messages/angular-messages",
    "angular-route": "../bower_components/angular-route/angular-route",
    "angular-material": "../bower_components/angular-material/angular-material",
    "angular-material-icons": "../bower_components/angular-material-icons/angular-material-icons",
    "download": "../bower_components/angular-download/angular-download",
    "tinycolor": "../bower_components/tinycolor/dist/tinycolor-min",
    "md-color-picker": "../bower_components/md-color-picker/dist/mdColorPicker",
    "xml2json": "../bower_components/x2js/xml2json",
    "angular-xml": "../bower_components/angular-xml/angular-xml",
    "clipboard": "../bower_components/clipboard/dist/clipboard.min",
    "ngclipboard": "../bower_components/ngclipboard/dist/ngclipboard.min",
    "dicomParser": "../bower_components/dicomParser/dist/dicomParser",
    "ng-file-upload": "../bower_components/ng-file-upload/ng-file-upload",
    "ngProgress": "../bower_components/ngprogress/build/ngProgress",
    "ajv": "../node_modules/ajv/dist/ajv.min",
    "JSONSemanticsCreator": "app"
  },
  shim: {
    "angular": {
      deps: ["jquery"]
    },
    "angular-material": {
      deps: ["angular", "angular-aria"]
    },
    "angular-material-icons": {
      deps: ["angular"]
    },
    "JSONSemanticsCreator": {
      deps: ["angular", "angular-route", "angular-material", "angular-messages", "angular-material-icons", "v-accordion",
             "angular-animate", "angular-xml", "md-color-picker", "ngclipboard", "ajv", "download", "ng-file-upload",
             "ngProgress"]
    },
    "angular-messages": {
      deps: ["angular"],
      exports: "ngMessages"
    },
    "angular-xml": {
      deps: ["angular", "xml2json"]
    },
    "angular-route": {
      deps: ["angular"]
    },
    "angular-animate": {
      deps: ["angular"]
    },
    "download": {
      deps: ["angular"]
    },
    "v-accordion": {
      deps: ["angular"]
    },
    "angular-aria": {
      deps: ["angular"]
    },
    "md-color-picker": {
      deps: ["angular", "tinycolor"]
    },
    "ngclipboard": {
      deps: ["angular", "clipboard"]
    },
    "ng-file-upload": {
      deps: ["angular"]
    },
    "ngProgress": {
      deps: ["angular"]
    }
  }
});

require(["clipboard"], function(clipboard) {
  window.Clipboard = clipboard;
});

require(["tinycolor"], function(tinycolor) {
  window.tinycolor = tinycolor;
});

require(["JSONSemanticsCreator"],
  function () {
    angular.bootstrap(document, ["JSONSemanticsCreator"]);
  }
);