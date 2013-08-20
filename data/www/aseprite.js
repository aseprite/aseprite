window.Aseprite = window.Aseprite || {};
(function(Aseprite) {
    'use strict';

    Aseprite.host = 'http://127.0.0.1:10453';

    Aseprite.get = function(uri, callback) {
        $.ajax({
            type: 'GET',
            url: Aseprite.host + uri,
            dataType: 'json',
            cache: false,
            success: function(data) {
                callback(data);
            }
        });
    }

    Aseprite.get_version = function(callback) {
        Aseprite.get('/version', callback);
    }
})(window.Aseprite);
