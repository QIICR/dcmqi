define(['ajv', 'dicomParser'], function (Ajv, dicomParser) {

  var user = 'qiicr';
  var rev = 'master';
  var webAssets = 'https://raw.githubusercontent.com/'+user+'/dcmqi/'+rev+'/doc/';
  // var webAssets = 'assets/doc/';

  var commonSchemaURL = webAssets + 'common-schema.json';
  var srCommonSchemaURL = webAssets + 'sr-common-schema.json';
  var segContextCommonSchemaURL = webAssets + 'segment-context-common-schema.json';

  var segSchemaURL = webAssets + 'seg-schema.json';
  var srSchemaURL = webAssets + 'sr-tid1500-schema.json';
  var pmSchemaURL = webAssets + 'pm-schema.json';
  var acSchemaURL = webAssets + 'anatomic-context-schema.json';
  var scSchemaURL = webAssets + 'segment-context-schema.json';

  var segSchemaExampleURL = webAssets + 'seg-example.json';
  var srSchemaExampleURL = webAssets + 'sr-tid1500-example.json';
  var pmSchemaExampleURL = webAssets + 'pm-example.json';

  var idRoot = 'https://raw.githubusercontent.com/qiicr/dcmqi/master/doc/';
  var segSchemaID = idRoot + 'seg-schema.json';
  var srSchemaID = idRoot + 'sr-tid1500-schema.json';
  var pmSchemaID = idRoot + 'pm-schema.json';
  var acSchemaID = idRoot + 'anatomic-context-schema.json';
  var scSchemaID = idRoot + 'segment-context-schema.json';

  var schemata = [];

  function Schema (name, id, url, refs, example) {
      this.name = name;
      this.id = id;
      this.url = url;
      this.refs = refs;
      this.example = example;
      schemata.push(this)
  }

  var segSchema = new Schema("Segmentation", segSchemaID, segSchemaURL, [commonSchemaURL], segSchemaExampleURL);
  var srSchema = new Schema("Structured Report TID 1500", srSchemaID, srSchemaURL,
    [commonSchemaURL, srCommonSchemaURL], srSchemaExampleURL);
  var pmSchema = new Schema("Parametric Map", pmSchemaID, pmSchemaURL, [commonSchemaURL], pmSchemaExampleURL);
  var acSchema = new Schema("Anatomic Context", acSchemaID, acSchemaURL, [commonSchemaURL, segContextCommonSchemaURL]);
  var scSchema = new Schema("Segment Context", scSchemaID, scSchemaURL, [commonSchemaURL, segContextCommonSchemaURL]);

  // var segSchemaID = webAssets + 'seg-schema.json';

  var anatomicRegionContextSources = [webAssets+'segContexts/AnatomicRegionAndModifier-DICOM-Master.json'];

  var segCategoryTypeContextSources = [webAssets+'segContexts/SegmentationCategoryTypeModifier-DICOM-Master.json',
                                       webAssets+'segContexts/SegmentationCategoryTypeModifier-SlicerGeneralAnatomy.json'];


  var app = angular.module('JSONSemanticsCreator', ['ngRoute', 'ngMaterial', 'ngMessages', 'ngMdIcons', 'vAccordion',
                                                    'ngAnimate', 'xml', 'ngclipboard', 'mdColorPicker', 'download',
                                                    'ngFileUpload', 'ngProgress', 'ui.ace']);


  app.config(function ($httpProvider) {
      $httpProvider.interceptors.push('xmlHttpInterceptor');
    });


  app.config(function($mdThemingProvider) {
    $mdThemingProvider.theme('default')
      .primaryPalette('green')
      .accentPalette('red');
  });


  app.config(function($routeProvider) {
    $routeProvider
      .when('/home', {
        templateUrl: 'home.html',
        controller: 'JSONSemanticsCreatorMainController'
      })
      .when('/seg', {
        templateUrl: 'seg.html',
        controller: 'JSONSemanticsCreatorController'
      })
      .when('/validators', {
        templateUrl: 'validators.html',
        controller: 'JSONValidatorController'
      })
      .otherwise({
        redirectTo: '/home'
      });
  });


  app.controller('JSONSemanticsCreatorMainController', ['$scope',
    function($scope) {
      $scope.headlineText = "DCMQI Meta Information Generators";
      $scope.toolTipDelay = 500;
  }]);


  app.controller('JSONValidatorController', ['$scope', '$mdToast', '$http',
    function($scope, $mdToast, $http) {

      $scope.schemata = schemata;
      $scope.schema = null;
      $scope.input = "";
      $scope.output = "";
      $scope.showExample = true;
      $scope.showSchema = false;
      $scope.exampleJson = "";
      $scope.schemaJson = "";

      var ajv = null;
      var schemaLoaded = false;
      var validate = undefined;

      $scope.onSchemaSelected = function () {
        schemaLoaded = false;
        validate = undefined;
        $scope.output = "";
        ajv = new Ajv({
          useDefaults: true,
          allErrors: true,
          loadSchema: loadSchema });

        angular.forEach($scope.schema.refs, function(value, key) {
          loadSchema(value, function(err, body) {
            if (body != undefined) {
              console.log("loading reference: " + value);
              ajv.addSchema( body.data);
            }
          });
        });
        loadSchema($scope.schema.url, function(err, body){
          if (body != undefined) {
            console.log("loading schema: " + $scope.schema.url);
            ajv.addSchema(body.data);
            schemaLoaded = true;
            console.log("Schema successfully loaded ");
            validate = ajv.compile({$ref: $scope.schema.id});
            $scope.onOutputChanged();
            if($scope.schema.example == undefined) {
              $scope.showExample = false;
            } else {
              loadSchema($scope.schema.example, function(err, body) {
                if (body != undefined) {
                  $scope.exampleJson = JSON.stringify(body.data, null, 2);
                } else {
                  $scope.exampleJson = "";
                }
              });
            }
            $scope.schemaJson = JSON.stringify(body.data, null, 2);
          }
        });
      };

      function loadSchema(uri, callback) {
        $http({
          method: 'GET',
          url: uri
        }).then(function successCallback(body) {
          callback(null, body);
        }, function errorCallback(response) {
          callback(response || new Error('Loading error: ' + response.statusCode));
        });
      }

      $scope.onOutputChanged = function(e) {
        var message = "";
        if ($scope.input.length > 0) {
          try {
            var parsedJSON = JSON.parse($scope.input);
            if (!schemaLoaded) {
              $scope.output = "Schema for validation was not loaded.";
            } else {
              $scope.input = JSON.stringify(parsedJSON, null, 2);
              if (validate(parsedJSON)) {
                message = "Schema is valid.";
              } else {
                message = "";
                angular.forEach(validate.errors, function (value, key) {
                  message += ajv.errorsText([value]) + "\n";
                });
              }
            }
          } catch(ex) {
            message = ex.message;
          }
        }
        $scope.output = message;
      };
  }]);


  app.controller('JSONSemanticsCreatorController', ['$scope', '$rootScope', '$http', '$log', '$mdToast', 'download',
                                                    'Upload', 'ngProgressFactory',
    function($scope, $rootScope, $http, $log, $mdToast, download, Upload, ngProgressFactory) {

      var self = this;
      self.segmentedPropertyCategory = null;
      self.segmentedPropertyType = null;
      self.segmentedPropertyTypeModifier = null;
      self.anatomicRegion = null;
      self.anatomicRegionModifier = null;
      var currentLabelID = 1;

      $scope.submitForm = function(isValid) {
        if (isValid) {
          self.createJSONOutput();
          hideToast();
        } else {
          self.showErrors();
        }
      };

      $scope.downloadFile = function() {
        download.fromData($scope.output, "text/json", $scope.seriesAttributes.ClinicalTrialSeriesID+".json");
      };

      $scope.progressbar = ngProgressFactory.createInstance();
      $scope.progressbar.setHeight('5px');

      function populateAttributesFromDICOM(file)
      {
        var reader = new FileReader();
        reader.onprogress = function(event) {
          if (event.lengthComputable) {
            $scope.progressbar.set(event.loaded/event.total);
          }
        };
        reader.onload = function(file) {
          $scope.progressbar.complete();
          var arrayBuffer = reader.result;
          var byteArray = new Uint8Array(arrayBuffer);
          var dataset = dicomParser.parseDicom(byteArray);
          var t = $scope.seriesAttributes;
          t.ContentCreatorName = getValueOrOld(t.ContentCreatorName, dataset, 'x00700084');
          t.ClinicalTrialSeriesID = getValueOrOld(t.ClinicalTrialSeriesID, dataset, 'x00120071');
          t.ClinicalTrialTimePointID = getValueOrOld(t.ClinicalTrialTimePointID, dataset, 'x00120050');
          t.SeriesDescription = getValueOrOld(t.SeriesDescription, dataset, 'x0008103E');
          t.SeriesNumber = getValueOrOld(t.SeriesNumber, dataset, 'x00200011');
          t.InstanceNumber = getValueOrOld(t.InstanceNumber, dataset, 'x00200013');
        };
        reader.readAsArrayBuffer(file);
      }

      function getValueOrOld(field, dataset, tag) {
        var value = dataset.string(tag);
        return value != undefined ? value : field;
      }

      $scope.$watch('file', function () {
        if ($scope.file != undefined) {
          $scope.dropZoneText = "DICOM file: " + $scope.file.name;
          populateAttributesFromDICOM($scope.file);
        } else {
          $scope.dropZoneText = "Auto-populate attributes: Drop DICOM image here or click to upload";
        }
      });

      $scope.validJSON = false;

      var ajv = new Ajv({
        useDefaults: true,
        allErrors: true,
        loadSchema: loadSchema });

      var schemaLoaded = false;
      var validate = undefined;
      var commonSchema = undefined;
      var segSchema = undefined;
      var segment = undefined;

      loadSchema(commonSchemaURL, function(err, body) {
        if (body != undefined) {
          commonSchema = body.data;
          ajv.addSchema(commonSchema);
        }
        loadSchema(segSchemaURL, function(err, body){
          if (body != undefined) {
            segSchema = body.data;
            ajv.addSchema(segSchema);
            schemaLoaded = true;
          }
          $scope.resetForm();
        });
      });

      function loadDefaultSeriesAttributes() {
        var doc = {};
        if (schemaLoaded) {
          validate = ajv.compile({$ref: segSchemaID});
          var valid = validate(doc);
          if (!valid) console.log(ajv.errorsText(validate.errors));
        }
        $scope.seriesAttributes = angular.extend({}, doc);
      }

      function loadAndValidateDefaultSegmentAttributes() {
        var doc = {
          "segmentAttributes": [[getDefaultSegmentAttributes()]]
        };
        if (schemaLoaded) {
          validate = ajv.compile({$ref: segSchemaID});
          var valid = validate(doc);
          if (!valid) console.log(ajv.errorsText(validate.errors));
        }
        return doc.segmentAttributes[0][0];
      }

      function loadSchema(uri, callback) {
        $http({
          method: 'GET',
          url: uri
        }).then(function successCallback(body) {
          callback(null, body);
        }, function errorCallback(response) {
          callback(response || new Error('Loading error: ' + response.statusCode));
        });
      }

      $scope.resetForm = function() {
        $scope.validJSON = false;
        currentLabelID = 1;
        loadDefaultSeriesAttributes();
        $scope.segments = [loadAndValidateDefaultSegmentAttributes()];
        $scope.segments[0].recommendedDisplayRGBValue = angular.extend({}, defaultRecommendedDisplayValue);
        $scope.output = "";
      };

      $scope.onOutputChanged = function() {
        if ($scope.output.length > 0) {
          try {
            var parsedJSON = JSON.parse($scope.output);
            if (!schemaLoaded) {
              showToast("Schema for validation was not loaded.");
              return;
            }
            var valid = validate(parsedJSON);
            $scope.output = JSON.stringify(parsedJSON, null, 2);
            if (valid) {
              hideToast();
              $scope.validJSON = true;
            } else {
              $scope.validJSON = false;
              showToast(ajv.errorsText(validate.errors));
            }
          } catch(ex) {
            $scope.validJSON = false;
            showToast(ex.message);
          }
        }
      };

      function showToast(content) {
        $mdToast.show(
          $mdToast.simple()
            .content(content)
            .action('OK')
            .position('bottom right')
            .hideDelay(100000)
        );
      }

      function hideToast() {
        $mdToast.hide();
      }

      var colorPickerDefaultOptions = {
        clickOutsideToClose: true,
        openOnInput: false,
        mdColorAlphaChannel: false,
        mdColorClearButton: false,
        mdColorSliders: false,
        mdColorHistory: false,
        mdColorGenericPalette: false,
        mdColorMaterialPalette:false,
        mdColorHex: false,
        mdColorHsl: false
      };

      var defaultRecommendedDisplayValue = {
        color: '',
        backgroundOptions: angular.extend({}, colorPickerDefaultOptions)
      };

      // TODO: populate that from schema?
      $scope.segmentAlgorithmTypes = [
        "MANUAL",
        "SEMIAUTOMATIC",
        "AUTOMATIC"
      ];

      $scope.isSegmentAlgorithmNameRequired = function(algorithmType) {
        return ["SEMIAUTOMATIC", "AUTOMATIC"].indexOf(algorithmType) > -1;
      };

      function getDefaultSegmentAttributes() {
        return {
          LabelID: currentLabelID,
          SegmentDescription: "",
          AnatomicRegionSequence: {},
          AnatomicRegionModifierSequence: {},
          SegmentedPropertyTypeModifierCodeSequence: {}
        };
      }

      $scope.segmentationContexts = [];
      $scope.selectedSegmentationCategoryContext = undefined;
      angular.forEach(segCategoryTypeContextSources, function(value, key) {
        $http.get(value).success(function (data) {
          $scope.segmentationContexts.push(
            {
              url: value,
              name: data.SegmentationCategoryTypeContextName
            });
        });
        if ($scope.segmentationContexts.length == 1)
          $scope.selectedSegmentationCategoryContext = $scope.segmentationContexts[0];
      });

      $scope.anatomicRegionContexts = [];
      $scope.selectedAnatomicRegionContext = undefined;
      angular.forEach(anatomicRegionContextSources, function(value, key) {
        $http.get(value).success(function (data) {
          $scope.anatomicRegionContexts.push(
            {
              url: value,
              name: data.AnatomicContextName
            });
          if (anatomicRegionContextSources.length == 1)
            $scope.selectedAnatomicRegionContext = $scope.anatomicRegionContexts[0];
        });
      });

      $scope.addSegment = function() {
        currentLabelID += 1;
        var segment = loadAndValidateDefaultSegmentAttributes();
        segment.recommendedDisplayRGBValue = angular.extend({}, defaultRecommendedDisplayValue);
        $scope.segments.push(segment);
        $scope.selectedIndex = $scope.segments.length-1;
      };

      $scope.removeSegment = function() {
        $scope.segments.splice($scope.selectedIndex, 1);
        if ($scope.selectedIndex-1 < 0)
          $scope.selectedIndex = 0;
        else
          $scope.selectedIndex -= 1;
        $scope.output = "";
      };

      $scope.previousSegment = function() {
        $scope.selectedIndex -= 1;
      };

      $scope.nextSegment = function() {
        $scope.selectedIndex += 1;
      };

      self.showErrors = function() {
        $scope.output = "";
        var firstError = $scope.jsonForm.$error.required[0];
        var elements = firstError.$name.split("_");
        var message = "[MISSING]: " + elements[0];
        if (elements[1] != undefined)
          message += " for segment with label id " + elements[1];
        showToast(message);
      };

      $scope.segmentAlreadyExists = function(segment) {
        var exists = false;
        angular.forEach($scope.segments, function(value, key) {
          if(value.LabelID == segment.LabelID && value != segment) {
            exists = true;
          }
        });
        return exists;
      };

      self.createJSONOutput = function() {

        var doc = {
          "ContentCreatorName": $scope.seriesAttributes.ContentCreatorName,
          "ClinicalTrialSeriesID" : $scope.seriesAttributes.ClinicalTrialSeriesID,
          "ClinicalTrialTimePointID" : $scope.seriesAttributes.ClinicalTrialTimePointID,
          "SeriesDescription" : $scope.seriesAttributes.SeriesDescription,
          "SeriesNumber" : $scope.seriesAttributes.SeriesNumber,
          "InstanceNumber" : $scope.seriesAttributes.InstanceNumber
        };

        if ($scope.seriesAttributes.BodyPartExamined.length > 0)
          doc["BodyPartExamined"] = $scope.seriesAttributes.BodyPartExamined;

        var segmentAttributes = [];
        angular.forEach($scope.segments, function(value, key) {
          var attributes = {};
          attributes["LabelID"] = value.LabelID;
          if (value.SegmentDescription.length > 0)
            attributes["SegmentDescription"] = value.SegmentDescription;
          if (value.SegmentAlgorithmType.length > 0)
            attributes["SegmentAlgorithmType"] = value.SegmentAlgorithmType;
          if (value.SegmentAlgorithmName != undefined && value.SegmentAlgorithmName.length > 0)
            attributes["SegmentAlgorithmName"] = value.SegmentAlgorithmName;
          if (value.anatomicRegion)
            attributes["AnatomicRegionSequence"] = getCodeSequenceAttributes(value.anatomicRegion);
          if (value.anatomicRegionModifier)
            attributes["AnatomicRegionModifierSequence"] = getCodeSequenceAttributes(value.anatomicRegionModifier);
          if (value.segmentedPropertyCategory)
            attributes["SegmentedPropertyCategoryCodeSequence"] = getCodeSequenceAttributes(value.segmentedPropertyCategory);
          if (value.segmentedPropertyType)
            attributes["SegmentedPropertyTypeCodeSequence"] = getCodeSequenceAttributes(value.segmentedPropertyType);
          if (value.segmentedPropertyTypeModifier)
            attributes["SegmentedPropertyTypeModifierCodeSequence"] = getCodeSequenceAttributes(value.segmentedPropertyTypeModifier);
          if (value.recommendedDisplayRGBValue.color)
            attributes["recommendedDisplayRGBValue"] = self.rgbToArray(value.recommendedDisplayRGBValue.color);
          segmentAttributes.push(attributes);
        });

        doc["segmentAttributes"] = [segmentAttributes];

        $scope.output = JSON.stringify(doc, null, 2);
        $scope.onOutputChanged();
      };

      self.rgbToArray = function(str) {
        var rgb = str.replace("rgb(", "").replace(")", "").split(", ");
        return [parseInt(rgb[0]), parseInt(rgb[1]), parseInt(rgb[2])];
      }

  }]);

  function getCodeSequenceAttributes(codeSequence) {
    if (codeSequence != null && codeSequence != undefined)
      return {"CodeValue":codeSequence.CodeValue,
              "CodingSchemeDesignator":codeSequence.CodingSchemeDesignator,
              "CodeMeaning":codeSequence.CodeMeaning}
  }


  app.controller('CodeSequenceBaseController',
    function($self, $scope, $rootScope, $http, $log, $timeout, $q) {
      var self = $self;

      self.simulateQuery = false;
      self.isDisabled    = false;

      self.querySearch   = querySearch;
      self.selectedItemChange = selectedItemChange;
      self.searchTextChange   = searchTextChange;
      self.selectionChangedEvent = "";

      self.getName = function() {
        return self.floatingLabel.replace(" ", "");
      };

      function querySearch (query) {
        var results = query ? self.mappedCodes.filter( createFilterFor(query) ) : self.mappedCodes,
          deferred;
        if (self.simulateQuery) {
          deferred = $q.defer();
          $timeout(function () { deferred.resolve( results ); }, Math.random() * 1000, false);
          return deferred.promise;
        } else {
          return results;
        }
      }

      function searchTextChange(text) {
        // $log.info('Text changed to ' + text);
      }

      function selectedItemChange(item) {
        $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem, segment:$scope.segment});
      }

      function createFilterFor(query) {
        var lowercaseQuery = angular.lowercase(query);
        return function filterFn(item) {
          return (item.value.indexOf(lowercaseQuery) != -1);
        };
      }

      self.codesList2CodeMeaning = function(list) {
        if(Object.prototype.toString.call( list ) != '[object Array]' ) {
          list = [list];
        }
        list.sort(function(a,b) {return (a.CodeMeaning > b.CodeMeaning) ? 1 : ((b.CodeMeaning > a.CodeMeaning) ? -1 : 0);});
        return list.map(function (code) {
          return {
            value: code.CodeMeaning.toLowerCase(),
            contextGroupName : code.contextGroupName,
            display: code.CodeMeaning,
            object: code
          };
        })
      };
    }
  );


  app.controller('AnatomicRegionController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.floatingLabel = "Anatomic Region";
    self.isDisabled = true;
    self.selectionChangedEvent = "AnatomicRegionSelectionChanged";

    self.selectedItemChange = function(item) {
      $scope.segment.anatomicRegion = item ? item.object : item;
      $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem, segment:$scope.segment});
    };

    $rootScope.$on("SegmentedPropertyCategorySelectionChanged", function(event, data) {
      if ($scope.segment.$$hashKey != data.segment.$$hashKey) {
        return;
      }
      if (data.item) {
        if(data.item.object.showAnatomy == "false") {
          self.isDisabled = true;
          self.searchText = undefined;
        } else {
          self.isDisabled = false;
        }
      } else {
        self.searchText = undefined;
      }
    });

    $scope.$watch('selectedAnatomicRegionContext', function () {
      if ($scope.selectedAnatomicRegionContext != undefined) {
        $http.get($scope.selectedAnatomicRegionContext.url).success(function (data) {
          $scope.anatomicCodes = data.AnatomicCodes.AnatomicRegion;
          self.mappedCodes = self.codesList2CodeMeaning($scope.anatomicCodes);
        });
      }
    });
  });


  app.controller('AnatomicRegionModifierController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.floatingLabel = "Anatomic Region Modifier";
    self.isDisabled = true;
    self.selectionChangedEvent = "AnatomicRegionModifierSelectionChanged";

    self.selectedItemChange = function(item) {
      $scope.segment.anatomicRegionModifier = item ? item.object : item;
      $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem, segment:$scope.segment});
    };

    $rootScope.$on("AnatomicRegionSelectionChanged", function(event, data) {
      if ($scope.segment.$$hashKey != data.segment.$$hashKey) {
        return;
      }
      if (data.item) {
        self.isDisabled = data.item.object.Modifier === undefined;
        if (data.item.object.Modifier === undefined) {
          self.searchText = undefined;
          self.mappedCodes = [];
        } else {
          self.mappedCodes = self.codesList2CodeMeaning(data.item.object.Modifier);
        }
      } else {
        self.mappedCodes = [];
        self.searchText = undefined;
        self.isDisabled = true;
      }
    });
  });


  app.controller('SegmentedPropertyCategoryCodeController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.required = true;
    self.floatingLabel = "Segmented Category";
    self.selectionChangedEvent = "SegmentedPropertyCategorySelectionChanged";

    self.selectedItemChange = function(item) {
      $scope.segment.segmentedPropertyCategory = item ? item.object : item;
      $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem, segment:$scope.segment});
    };

    $scope.$watch('selectedSegmentationCategoryContext', function () {
      if ($scope.selectedSegmentationCategoryContext != undefined) {

        $http.get($scope.selectedSegmentationCategoryContext.url).success(function (data) {
          $scope.segmentationCodes = data.SegmentationCodes.Category;
          self.mappedCodes = self.codesList2CodeMeaning($scope.segmentationCodes);
        });
      }
    });
  });


  app.controller('SegmentedPropertyTypeController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.required = true;
    self.floatingLabel = "Segmented Property Type";
    self.isDisabled = true;
    self.selectionChangedEvent = "SegmentedPropertyTypeSelectionChanged";

    self.selectedItemChange = function(item) {
      $scope.segment.segmentedPropertyType = item ? item.object : item;
      $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem, segment:$scope.segment});
      if (self.selectedItem === null) {
        $scope.segment.recommendedDisplayRGBValue.color = "";
        $scope.segment.hasRecommendedColor = false;
      }
      else if (self.selectedItem.object.recommendedDisplayRGBValue != undefined) {
        $scope.segment.hasRecommendedColor = true;
        var rgb = self.selectedItem.object.recommendedDisplayRGBValue;
        $scope.segment.recommendedDisplayRGBValue.color = 'rgb('+rgb[0]+', '+rgb[1]+', '+rgb[2]+')';
      }
    };

    $rootScope.$on("SegmentedPropertyCategorySelectionChanged", function(event, data) {
      if ($scope.segment.$$hashKey != data.segment.$$hashKey) {
        return;
      }
      if (data.item) {
        self.isDisabled = data.item.object.Type === undefined;
        if (data.item.object.Type === undefined) {
          self.searchText = undefined;
          self.mappedCodes = [];
        } else {
          self.mappedCodes = self.codesList2CodeMeaning(data.item.object.Type);
        }
      } else {
        self.mappedCodes = [];
        self.searchText = undefined;
        self.isDisabled = true;
      }
    });
  });


  app.controller('SegmentedPropertyTypeModifierController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.floatingLabel = "Segmented Property Type Modifier";
    self.isDisabled = true;
    self.selectionChangedEvent = "SegmentedPropertyTypeModifierSelectionChanged";

    self.selectedItemChange = function(item) {
      $scope.segment.segmentedPropertyTypeModifier = item ? item.object : item;
      $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem, segment:$scope.segment});
    };

    $rootScope.$on("SegmentedPropertyTypeSelectionChanged", function(event, data) {
      if ($scope.segment.$$hashKey != data.segment.$$hashKey) {
        return;
      }
      if (data.item) {
        self.isDisabled = data.item.object.Modifier === undefined;
        if (data.item.object.Modifier === undefined) {
          self.searchText = undefined;
          self.mappedCodes = [];
        } else {
          self.mappedCodes = self.codesList2CodeMeaning(data.item.object.Modifier);
        }
      } else {
        self.mappedCodes = [];
        self.searchText = undefined;
        self.isDisabled = true;
      }
    });
  });


  app.directive('customAutocomplete', function() {
    return {
      templateUrl: 'custom-autocomplete.html'
    };
  });


  app.directive("nonExistentLabel", function() {
    return {
      restrict: "A",

      require: "ngModel",

      link: function(scope, element, attributes, ngModel) {
        ngModel.$validators.nonExistentLabel = function(modelValue) {
          var exists = false;
          angular.forEach(scope.segments, function(value, key) {
            if(value.LabelID == modelValue && value != scope.segment) {
              exists = true;
            }
          });
          return !exists;
        }
      }
    };
  });


  app.directive('resize', function ($window) {
    return {
      link: function postLink(scope, elem, attrs) {

        scope.onResizeFunction = function(element) {
          var toolbar = document.getElementById('toolbar');
          element.windowHeight = $window.innerHeight - toolbar.clientHeight;
          var newHeight = element.windowHeight-$(toolbar).height()/2;
          $(element).height(newHeight);
        };

        scope.onResizeFunction(elem);

        angular.element($window).bind('resize', function() {
          scope.onResizeFunction(elem);
          scope.$apply();
        });
      }
    };
  });

});
