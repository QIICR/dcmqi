define(['ajv'], function (Ajv) {

  var idRoot = 'https://raw.githubusercontent.com/qiicr/dcmqi/master/doc/';

  var schemasURL = idRoot + 'schemas/';
  var examplesURL = idRoot + 'examples/';

  var apiPrefix = 'https://api.github.com/repos/QIICR/dcmqi/contents/doc/';
  var apiSchemasRoot = apiPrefix + 'schemas';
  var apiExamplesRoot = apiPrefix + 'examples';

  function Schema (name, filename, examples) {
    this.name = name;
    this.id = schemasURL + filename;
    this.url = schemasURL + filename;
    this.examples = examples;
  }

  function Example (name, filename) {
    this.name = name;
    this.url = examplesURL + filename;
  }

  var anatomicRegionContextSources = [idRoot+'segContexts/AnatomicRegionAndModifier-DICOM-Master.json'];
  var segCategoryTypeContextSources = [idRoot+'segContexts/SegmentationCategoryTypeModifier-DICOM-Master.json',
                                       idRoot+'segContexts/SegmentationCategoryTypeModifier-SlicerGeneralAnatomy.json'];


  var dependencies = ['ngRoute', 'ngMaterial', 'ngMessages', 'ngMdIcons', 'vAccordion', 'ngAnimate', 'xml',
                      'ngclipboard', 'mdColorPicker', 'download', 'ui.ace'];

  var app = angular.module('JSONSemanticsCreator', dependencies)
    .controller('MainController', MainController)
    .controller('JSONValidatorController', JSONValidatorController)
    .controller('MetaCreatorBaseController', MetaCreatorBaseController)
    .controller('SegmentationMetaCreatorController', SegmentationMetaCreatorController)
    .controller('CodeSequenceBaseController', CodeSequenceBaseController)
    .controller('AnatomicRegionController', AnatomicRegionController)
    .controller('AnatomicRegionModifierController', AnatomicRegionModifierController)
    .controller('SegmentedPropertyCategoryCodeController', SegmentedPropertyCategoryCodeController)
    .controller('SegmentedPropertyTypeController', SegmentedPropertyTypeController)
    .controller('SegmentedPropertyTypeModifierController', SegmentedPropertyTypeModifierController);


  app.service('ResourceLoaderService', ['$http', function($http) {

    var self = this;
    this.loadedReferences = {};

    this.loadSchema = function(uri, callback) {
      for (var key in self.loadedReferences) {
        if (key == uri) {
          callback(null, uri, self.loadedReferences[key]);
          return;
        }
      }
      $http({
        method: 'GET',
        url: uri
      }).then(function successCallback(body) {
        self.loadedReferences[uri] = body;
        callback(null, uri, body);
      }, function errorCallback(response) {
        callback(response || new Error('Loading error: ' + response.statusCode));
      });
    };

    this.loadResource = function(uri, callback) {
      $http({
        method: 'GET',
        url: uri
      }).then(function successCallback(body) {
        callback(null, uri, body);
      }, function errorCallback(response) {
        callback(response || new Error('Loading error: ' + response.statusCode));
      });
    };

    this.loadExample = function (uri, callback) {
      this.loadSchema(uri, callback);
    };

    this.loadSchemaWithReferences = function(url, callback) {
      self.loadSchema(url, function(err, uri, body){
        if (body != undefined) {
          console.log("loading schema: " + url);

          var references = self.findReferences(body.data);
          console.log(references);
          self.loadReferences(references, [url], callback);
        }
      });
    };

    this.loadReferences = function(references, loadedReferenceURLs, callback) {
      var numLoadedReferences = 0;
      var subReferences = [];
      angular.forEach(references, function(reference, key) {
        self.loadSchema(reference, function(err, uri, body) {
          if (body != undefined) {
            console.log("loading reference: " + reference);

            loadedReferenceURLs.push(uri);
            numLoadedReferences += 1;

            subReferences = subReferences.concat(self.findReferences(body)).filter(function(x, i, a) {
              return a.indexOf(x) == i;
            }).filter(function(x) {
              return loadedReferenceURLs.indexOf(x) < 0;
            });

            if (subReferences.length)
              console.log("sub references:" + subReferences);
            if (numLoadedReferences == references.length) {
              if (subReferences.length > 0) {
                self.loadReferences(subReferences, loadedReferenceURLs, callback)
              } else {
                callback(loadedReferenceURLs);
              }
            }
          }
        });
      });
    };

    this.findReferences = function(jsonObject) {
      // http://stackoverflow.com/questions/921789/how-to-loop-through-plain-javascript-object-with-objects-as-members
      var references = [];
      for (var key in jsonObject) {
        if (!jsonObject.hasOwnProperty(key)) continue;
        var obj = jsonObject[key];
        if (key == "$ref") {
          var reference = obj.substring(0, obj.indexOf('#'));
          if(reference.length > 0)
            references.push(reference);
          continue;
        }
        if (typeof obj== 'object' && obj!= null) {
          references = references.concat(self.findReferences(obj));
        }
      }
      return references.filter(function(x, i, a) {return a.indexOf(x) == i;});
    };

  }]);


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
        controller: 'MainController'
      })
      .when('/seg', {
        templateUrl: 'seg.html',
        controller: 'SegmentationMetaCreatorController'
      })
      .when('/pm', {
        templateUrl: 'pm.html',
        controller: 'ParametricMapMetaCreatorController'
      })
      .when('/validators', {
        templateUrl: 'validators.html',
        controller: 'JSONValidatorController'
      })
      .otherwise({
        redirectTo: '/home'
      });
  });


  function MainController($scope) {
    $scope.headlineText = "DCMQI Meta Information Generators";
    $scope.toolTipDelay = 500;
  }


  function JSONValidatorController($scope, ResourceLoaderService) {

    $scope.schemata = [];
    $scope.schema = undefined;

    ResourceLoaderService.loadResource(apiSchemasRoot, function(err, uri, body) {
      angular.forEach(body.data, function(value, key) {
        if(value.name.search("common") == -1) {
          $scope.schemata.push(new Schema(value.name.replace(".json", ""), value.name, []))
        }
      });
      ResourceLoaderService.loadResource(apiExamplesRoot, function(err1, uri1, body1) {
        angular.forEach(body1.data, function(example, key) {
          ResourceLoaderService.loadResource(examplesURL+example.name, function(err2, uri2, body2) {
            var schemaUrl = body2.data["@schema"].replace("#","");
            angular.forEach($scope.schemata, function(schema, key) {
              if(schemaUrl == schema.url) {
                schema.examples.push(new Example(example.name.replace(".json", ""), example.name));
              }
            });
          });
        });
      });
    });

    $scope.example = "";
    $scope.input = "";
    $scope.output = "";
    $scope.showExample = false;
    $scope.showSchema = false;
    $scope.exampleJson = "";
    $scope.schemaJson = "";

    var ajv = null;
    var schemaLoaded = false;
    var validate = undefined;
    var loadSchema = ResourceLoaderService.loadSchema;

    $scope.onSchemaSelected = function () {
      if($scope.schema == undefined)
        return;
      schemaLoaded = false;
      validate = undefined;
      $scope.output = "";

      ResourceLoaderService.loadSchemaWithReferences($scope.schema.url, onFinishedLoadingReferences);
    };

    $scope.onExampleSelected = function () {
      if ($scope.example == undefined)
        return;
      ResourceLoaderService.loadExample($scope.example.url, function(err, uri, body) {
        if (body != undefined) {
          $scope.exampleJson = JSON.stringify(body.data, null, 2);
        } else {
          $scope.exampleJson = "";
        }
      });
    };

    function onFinishedLoadingReferences(loadedURLs) {
      ajv = new Ajv({
        useDefaults: true,
        allErrors: true,
        loadSchema: loadSchema
      });

      angular.forEach(loadedURLs, function(value, key) {
        console.log("adding schema from url: " + value);
        var body = ResourceLoaderService.loadedReferences[value];
        ajv.addSchema(body.data);
      });

      schemaLoaded = true;
      validate = ajv.compile({$ref: $scope.schema.id});
      $scope.onOutputChanged();
      if($scope.schema && $scope.schema.examples.length > 0) {
        $scope.example = $scope.schema.examples[0];
        $scope.onExampleSelected();
      } else {
        $scope.example = undefined;
        $scope.exampleJson = "";
        $scope.showExample = false;
      }
      var body = ResourceLoaderService.loadedReferences[$scope.schema.url];
      $scope.schemaJson = JSON.stringify(body.data, null, 2);
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
              message = "Json input is valid.";
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
  }

  function NotImplementedError(message) {
    this.name = "NotImplementedError";
    this.message = (message || "");
  }
  NotImplementedError.prototype = Error.prototype;


  function MetaCreatorBaseController(vm, $scope, $rootScope, $mdToast, download, ResourceLoaderService) {

    $scope.init = function() {
      console.log("MetaCreatorBaseController: called base init");
      vm.loadSchema = ResourceLoaderService.loadSchema;
      vm.ajv = new Ajv({
        useDefaults: true,
        allErrors: true,
        loadSchema: vm.loadSchema
      });

      vm.schemaLoaded = false;
      vm.validate = undefined;

      if (vm.initializeValidationSchema === undefined)
        throw new NotImplementedError("Method initializeValidationSchema needs to be implemented by all child classes!");
      else
        vm.initializeValidationSchema();

      $scope.validJSON = false;
    };

    $scope.submitForm = function(isValid) {
      if (isValid) {
        if (vm.createJSONOutput === undefined)
          throw new NotImplementedError("Method createJSONOutput needs to be implemented by all child classes!");
        else
          vm.createJSONOutput();
        vm.hideToast();
      } else {
        vm.showErrors();
      }
    };

    vm.showToast = function(content) {
      $mdToast.show(
        $mdToast.simple()
          .content(content)
          .action('OK')
          .position('bottom right')
          .hideDelay(100000)
      );
    };

    vm.hideToast = function() {
      $mdToast.hide();
    };

    vm.showErrors = function() {
      $scope.output = "";
      var firstError = $scope.jsonForm.$error.required[0];
      var elements = firstError.$name.split("_");
      var message = "[MISSING]: " + elements[0];
      if (elements[1] != undefined)
        message += " for segment with label id " + elements[1];
      vm.showToast(message);
    };

    $scope.onOutputChanged = function() {
      if ($scope.output.length > 0) {
        try {
          var parsedJSON = JSON.parse($scope.output);
          if (!vm.schemaLoaded) {
            vm.showToast("Schema for validation was not loaded.");
            return;
          }
          var valid = vm.validate(parsedJSON);
          $scope.output = JSON.stringify(parsedJSON, null, 2);
          if (valid) {
            vm.hideToast();
            $scope.validJSON = true;
          } else {
            $scope.validJSON = false;
            vm.showToast(vm.ajv.errorsText(vm.validate.errors));
          }
        } catch(ex) {
          $scope.validJSON = false;
          vm.showToast(ex.message);
        }
      }
    };

    $scope.downloadFile = function() {
      download.fromData($scope.output, "text/json", $scope.seriesAttributes.ClinicalTrialSeriesID+".json");
    };

    vm.getCodeSequenceAttributes = function(codeSequence) {
      if (codeSequence != null && codeSequence != undefined)
        return {"CodeValue":codeSequence.CodeValue,
                "CodingSchemeDesignator":codeSequence.CodingSchemeDesignator,
                "CodeMeaning":codeSequence.CodeMeaning}
    };

    $scope.init();
  }

  function SegmentationMetaCreatorController($scope, $rootScope, $controller, $http, ResourceLoaderService) {
    var vm = this;

    var init = function() {
      vm.schema = new Schema('Segmentation', 'seg-schema.json', []);
      vm.segmentedPropertyCategory = null;
      vm.segmentedPropertyType = null;
      vm.segmentedPropertyTypeModifier = null;
      vm.anatomicRegion = null;
      vm.anatomicRegionModifier = null;
      vm.currentLabelID = 1;
      $controller('MetaCreatorBaseController', {vm: vm, $scope: $scope, $rootScope: $rootScope});

      loadSegmentationContexts();
      loadAnatomicRegionContexts();
    };

    vm.initializeValidationSchema = function() {
      ResourceLoaderService.loadSchemaWithReferences(vm.schema.url, function(loadedURLs){
        angular.forEach(loadedURLs, function(value, key) {
          console.log("adding schema from url: " + value);
          var body = ResourceLoaderService.loadedReferences[value];
          vm.ajv.addSchema(body.data);
        });
        vm.schemaLoaded = true;
        $scope.resetForm();
      });
    };

    function loadDefaultSeriesAttributes() {
      var doc = {};
      if (vm.schemaLoaded) {
        vm.validate = vm.ajv.compile({$ref: vm.schema.id});
        var valid = vm.validate(doc);
        if (!valid) console.log(vm.ajv.errorsText(vm.validate.errors));
      }
      $scope.seriesAttributes = angular.extend({}, doc);
    }

    function loadAndValidateDefaultSegmentAttributes() {
      var doc = {
        "segmentAttributes": [[vm.getDefaultSegmentAttributes()]]
      };
      if (vm.schemaLoaded) {
        vm.validate = vm.ajv.compile({$ref: vm.schema.id});
        var valid = vm.validate(doc);
        if (!valid) console.log(vm.ajv.errorsText(vm.validate.errors));
      }
      return doc.segmentAttributes[0][0];
    }

    $scope.resetForm = function() {
      $scope.validJSON = false;
      vm.currentLabelID = 1;
      loadDefaultSeriesAttributes();
      $scope.segments = [loadAndValidateDefaultSegmentAttributes()];
      $scope.segments[0].recommendedDisplayRGBValue = angular.extend({}, defaultRecommendedDisplayValue);
      $scope.output = "";
    };

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

    vm.getDefaultSegmentAttributes = function() {
      return {
        labelID: vm.currentLabelID,
        SegmentDescription: "",
        AnatomicRegionSequence: {},
        AnatomicRegionModifierSequence: {},
        SegmentedPropertyTypeModifierCodeSequence: {}
      };
    };

    function loadSegmentationContexts() {
      $scope.segmentationContexts = [];
      $scope.selectedSegmentationCategoryContext = undefined;
      angular.forEach(segCategoryTypeContextSources, function (value, key) {
        $http.get(value).success(function (data) {
          $scope.segmentationContexts.push({
            url: value,
            name: data.SegmentationCategoryTypeContextName
          });
        });
        if ($scope.segmentationContexts.length == 1)
          $scope.selectedSegmentationCategoryContext = $scope.segmentationContexts[0];
      });
    }

    function loadAnatomicRegionContexts() {
      $scope.anatomicRegionContexts = [];
      $scope.selectedAnatomicRegionContext = undefined;
      angular.forEach(anatomicRegionContextSources, function (value, key) {
        $http.get(value).success(function (data) {
          $scope.anatomicRegionContexts.push({
            url: value,
            name: data.AnatomicContextName
          });
          if (anatomicRegionContextSources.length == 1)
            $scope.selectedAnatomicRegionContext = $scope.anatomicRegionContexts[0];
        });
      });
    }

    $scope.addSegment = function() {
      vm.currentLabelID += 1;
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

    $scope.segmentAlreadyExists = function(segment) {
      var exists = false;
      angular.forEach($scope.segments, function(value, key) {
        if(value.labelID == segment.labelID && value != segment) {
          exists = true;
        }
      });
      return exists;
    };

    vm.createJSONOutput = function() {

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
        attributes["labelID"] = value.labelID;
        if (value.SegmentDescription.length > 0)
          attributes["SegmentDescription"] = value.SegmentDescription;
        if (value.SegmentAlgorithmType.length > 0)
          attributes["SegmentAlgorithmType"] = value.SegmentAlgorithmType;
        if (value.SegmentAlgorithmName != undefined && value.SegmentAlgorithmName.length > 0)
          attributes["SegmentAlgorithmName"] = value.SegmentAlgorithmName;
        if (value.anatomicRegion)
          attributes["AnatomicRegionSequence"] = vm.getCodeSequenceAttributes(value.anatomicRegion);
        if (value.anatomicRegionModifier)
          attributes["AnatomicRegionModifierSequence"] = vm.getCodeSequenceAttributes(value.anatomicRegionModifier);
        if (value.segmentedPropertyCategory)
          attributes["SegmentedPropertyCategoryCodeSequence"] = vm.getCodeSequenceAttributes(value.segmentedPropertyCategory);
        if (value.segmentedPropertyType)
          attributes["SegmentedPropertyTypeCodeSequence"] = vm.getCodeSequenceAttributes(value.segmentedPropertyType);
        if (value.segmentedPropertyTypeModifier)
          attributes["SegmentedPropertyTypeModifierCodeSequence"] = vm.getCodeSequenceAttributes(value.segmentedPropertyTypeModifier);
        if (value.recommendedDisplayRGBValue.color)
          attributes["recommendedDisplayRGBValue"] = vm.rgbToArray(value.recommendedDisplayRGBValue.color);
        segmentAttributes.push(attributes);
      });

      doc["segmentAttributes"] = [segmentAttributes];

      $scope.output = JSON.stringify(doc, null, 2);
      $scope.onOutputChanged();
    };

    vm.rgbToArray = function(str) {
      var rgb = str.replace("rgb(", "").replace(")", "").split(", ");
      return [parseInt(rgb[0]), parseInt(rgb[1]), parseInt(rgb[2])];
    };

    init();
  }


  function CodeSequenceBaseController($self, $scope, $rootScope, $timeout, $q) {
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


  function AnatomicRegionController($scope, $rootScope, $http, $controller) {
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
  }


  function AnatomicRegionModifierController($scope, $rootScope, $controller) {
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
  }


  function SegmentedPropertyCategoryCodeController($scope, $rootScope, $http, $controller) {
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
  }


  function SegmentedPropertyTypeController($scope, $rootScope, $controller) {
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
  }


  function SegmentedPropertyTypeModifierController($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
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
  }


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
            if(value.labelID == modelValue && value != scope.segment) {
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
          var childrenHeight = 0;
          angular.forEach($(element).parent().children(), function (child, key) {
            if (child.id != $(element)[0].id)
              childrenHeight += $(child).height();
          });
          newHeight -= childrenHeight;
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
