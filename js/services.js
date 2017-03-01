define([], function () {

  angular.module('app.services', [])
    .service('ResourceLoaderService', ['$http', function($http) {

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
            // console.log("loading schema: " + url);
            var references = self.findReferences(body.data);
            // console.log(references);
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
              // console.log("loading reference: " + reference);

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

});