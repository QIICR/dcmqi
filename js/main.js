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
    "tinycolor": "../bower_components/tinycolor/dist/tinycolor-min",
    "mdColorPicker": "../bower_components/md-color-picker/dist/mdColorPicker",
    "xml2json": "../bower_components/x2js/xml2json",
    "angular-xml": "../bower_components/angular-xml/angular-xml",
    "ajv": "../bower_components/ajv/lib/ajv",
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
             "angular-animate", "angular-xml"]
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
    "v-accordion": {
      deps: ["angular"]
    },
    "angular-aria": {
      deps: ["angular"]
    },
    "mdColorPicker": {
      deps: ["angular", "tinycolor"]
    }
  }
});

require(["JSONSemanticsCreator"],
  function () {
    angular.bootstrap(document, ["JSONSemanticsCreator"]);
  }
);