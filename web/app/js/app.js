(function(angular) {

  var anatomicRegionXMLPath = 'assets/AnatomicRegionAndModifier.xml';
  var segmentationCodesXMLPath = 'assets/SegmentationCategoryTypeModifier.xml';

  var app = angular.module('JSONSemanticsCreator', ['ngMaterial', 'ngMessages', 'ngMdIcons', 'xml'])
    .config(function ($httpProvider) {
      $httpProvider.interceptors.push('xmlHttpInterceptor');
    });

  app.controller('JSONSemanticsCreatorController', ['$scope', '$rootScope', '$log',
    function($scope, $rootScope, $log) {

      var self = this;
      self.segmentedPropertyCategory = null;
      self.segmentedPropertyType = null;
      self.segmentedPropertyTypeModifier = null;
      self.anatomicRegion = null;
      self.anatomicRegionModifier = null;

      $scope.submitted = false;

      $rootScope.$on("SegmentedPropertyCategorySelectionChanged", function(event, data) {
        if (data.item) {
          self.segmentedPropertyCategory = data.item.object;
        }
        $log.info(self.segmentedPropertyCategory)
      });

      $rootScope.$on("SegmentedPropertyTypeSelectionChanged", function(event, data) {
        if (data.item) {
          self.segmentedPropertyType = data.item.object;
        }
        $log.info(self.segmentedPropertyType)
      });

      $rootScope.$on("SegmentedPropertyTypeModifierSelectionChanged", function(event, data) {
        if (data.item) {
          self.segmentedPropertyTypeModifier = data.item.object;
          $log.info(self.segmentedPropertyTypeModifier)
        }
      });

      $rootScope.$on("AnatomicRegionSelectionChanged", function(event, data) {
        if (data.item) {
          self.anatomicRegion = data.item.object;
        }
        $log.info(self.anatomicRegion)
      });

      $rootScope.$on("AnatomicRegionModifierSelectionChanged", function(event, data) {
        if (data.item) {
          self.anatomicRegionModifier = data.item.object;
        }
        $log.info(self.anatomicRegionModifier)
      });

      $scope.submitForm = function(isValid) {
        if (isValid) {
          self.createJSONOutput();
        } else {
          $scope.output = "";
        }
      };

      $scope.seriesAttributes = {
        ReaderID : "Reader1",
        SessionID : "Session1",
        TimePointID : "1",
        SeriesDescription : "Segmentation",
        SeriesNumber : 300,
        InstanceNumber : 1,
        BodyPartExamined :  ""
      };

      $scope.segmentAttributes = {
        LabelID: 1,
        SegmentDescription: "" // not required
      };

      self.segmentAttributes = [];

      self.previousSegmentExists = false;
      self.nextSegmentExists = false;

      $scope.output = {};

      $scope.next = function() {
        // TODO: create new segment entry here take current input data and add to class
        // if ()
        self.previousSegmentExists = true;
      };

      $scope.previous = function() {
        self.nextSegmentExists = false;
      };

      self.createJSONOutput = function() {

        var seriesAttributes = {
          "ReaderID": $scope.seriesAttributes.ReaderID,
          "SessionID" : $scope.seriesAttributes.SessionID,
          "TimePointID" : $scope.seriesAttributes.TimePointID,
          "SeriesDescription" : $scope.seriesAttributes.SeriesDescription,
          "SeriesNumber" : $scope.seriesAttributes.SeriesNumber,
          "InstanceNumber" : $scope.seriesAttributes.InstanceNumber
        };

        if ($scope.seriesAttributes.BodyPartExamined.length > 0)
          seriesAttributes["BodyPartExamined"] = $scope.seriesAttributes.BodyPartExamined;

        var segmentAttributes = {};

        segmentAttributes["LabelID"] = $scope.segmentAttributes.LabelID;
        if ($scope.segmentAttributes.SegmentDescription.length > 0)
          segmentAttributes["SegmentDescription"] = $scope.segmentAttributes.SegmentDescription;
        if (self.anatomicRegion)
          segmentAttributes["AnatomicRegion"] = getCodeSequenceAttributes(self.anatomicRegion);
        if (self.anatomicRegionModifier)
          segmentAttributes["AnatomicRegionModifier"] = getCodeSequenceAttributes(self.anatomicRegionModifier);
        if (self.segmentedPropertyCategory)
          segmentAttributes["SegmentedPropertyCategoryCode"] = getCodeSequenceAttributes(self.segmentedPropertyCategory);
        if (self.segmentedPropertyType)
          segmentAttributes["SegmentedPropertyType"] = getCodeSequenceAttributes(self.segmentedPropertyType);
        if (self.segmentedPropertyTypeModifier)
          segmentAttributes["SegmentedPropertyTypeModifier"] = getCodeSequenceAttributes(self.segmentedPropertyTypeModifier);

        var doc = {
          "seriesAttributes": seriesAttributes,
          "segmentAttributes": segmentAttributes
        };

          $scope.output = doc;
      };
  }]);

  function getCodeSequenceAttributes(codeSequence) {
    if (codeSequence != null && codeSequence != undefined)
      return {"codeValue":codeSequence._codeValue,
              "codingSchemeDesignator":codeSequence._codingScheme,
              "codeMeaning":codeSequence._codeMeaning}

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
        // $log.info('Item changed to ' + JSON.stringify(item));
        $rootScope.$emit(self.selectionChangedEvent, {item:self.selectedItem});
      }

      function createFilterFor(query) {
        var lowercaseQuery = angular.lowercase(query);
        return function filterFn(item) {
          return (item.value.indexOf(lowercaseQuery) != -1);
        };
      }

      self.codesList2codeMeaning = function(list) {
        if(Object.prototype.toString.call( list ) != '[object Array]' ) {
          list = [list];
        }
        list.sort(function(a,b) {return (a._codeMeaning > b._codeMeaning) ? 1 : ((b._codeMeaning > a._codeMeaning) ? -1 : 0);});
        return list.map(function (code) {
          return {
            value: code._codeMeaning.toLowerCase(),
            contextGroupName : code._contextGroupName,
            display: code._codeMeaning,
            object: code
          };
        })
      }
    }
  );


  app.controller('AnatomicRegionController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.floatingLabel = "Anatomic Region";
    self.selectionChangedEvent = "AnatomicRegionSelectionChanged";

    $http.get(anatomicRegionXMLPath).success(function (data) {
      $scope.anatomicCodes = data.AnatomicCodes.AnatomicRegion;
      self.mappedCodes = self.codesList2codeMeaning($scope.anatomicCodes);
    });
  });


  app.controller('AnatomicRegionModifierController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.floatingLabel = "Modifier";
    self.isDisabled = true;
    self.selectionChangedEvent = "AnatomicRegionModifierSelectionChanged";

    $rootScope.$on("AnatomicRegionSelectionChanged", function(event, data) {
      if (data.item) {
        self.isDisabled = data.item.object.Modifier === undefined;
        if (data.item.object.Modifier === undefined) {
          self.searchText = undefined;
          self.mappedCodes = [];
        } else {
          self.mappedCodes = self.codesList2codeMeaning(data.item.object.Modifier);
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
    self.floatingLabel = "Segmented Category";
    self.selectionChangedEvent = "SegmentedPropertyCategorySelectionChanged";

    $http.get(segmentationCodesXMLPath).success(function (data) {
      $scope.segmentationCodes = data.SegmentationCodes.Category;
      self.mappedCodes = self.codesList2codeMeaning($scope.segmentationCodes);
    });
  });


  app.controller('SegmentedPropertyTypeController', function($scope, $rootScope, $http, $log, $timeout, $q, $controller) {
    $controller('CodeSequenceBaseController', {$self:this, $scope: $scope, $rootScope: $rootScope});
    var self = this;
    self.floatingLabel = "Type";
    self.isDisabled = true;
    self.selectionChangedEvent = "SegmentedPropertyTypeSelectionChanged";

    $rootScope.$on("SegmentedPropertyCategorySelectionChanged", function(event, data) {
      if (data.item) {
        self.isDisabled = data.item.object.Type === undefined;
        if (data.item.object.Type === undefined) {
          self.searchText = undefined;
          self.mappedCodes = [];
        } else {
          // angular.forEach(data.item.object.Type, function(value, key) {
          //   console.log(value._codeMeaning);
          // });
          self.mappedCodes = self.codesList2codeMeaning(data.item.object.Type);
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
    self.floatingLabel = "Modifier";
    self.isDisabled = true;
    self.selectionChangedEvent = "SegmentedPropertyTypeModifierSelectionChanged";

    $rootScope.$on("SegmentedPropertyTypeSelectionChanged", function(event, data) {
      if (data.item) {
        self.isDisabled = data.item.object.Modifier === undefined;
        if (data.item.object.Modifier === undefined) {
          self.searchText = undefined;
          self.mappedCodes = [];
        } else {
          self.mappedCodes = self.codesList2codeMeaning(data.item.object.Modifier);
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
  
})(window.angular);