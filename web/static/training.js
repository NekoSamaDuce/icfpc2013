var setStatus = function(obj) {
  $('#status').text(obj);
  console.log(obj);
};

var onGenerate = function() {
  $('#evals').empty();
  var size = $('input[name=size]').val();
  var operators = $('input[name=operators]').val();
  request = {size: size, operators: operators}
  $.post('/training/generate', data=request).then(
    function(data) {
      setStatus('generate success');
      $('input[name=id]').val(data['id']);
      $('input[name=size]').val(data['size']);
      $('input[name=operators]').val(data['operators'].join(', '));
    },
    function(response) {
      setStatus(response.responseText);
    });
};

var onEval = function() {
  var id = $('input[name=id]').val();
  var argument = $('input[name=argument]').val();
  request = {id: id, argument: argument};
  $.post('/training/eval', data=request).then(
    function(data) {
      setStatus('eval success');
      var cases = data['cases'];
      for (var i = 0; i < cases.length; ++i) {
        var c = cases[i];
        var li = $('<li>').text(c['argument'] + ' => ' + c['output']);
        $('#evals').append(li);
      }
    },
    function(response) {
      setStatus(response.responseText);
    });
};

var onGuess = function() {
  var id = $('input[name=id]').val();
  var program = $('input[name=program]').val();
  request = {id: id, program: program};
  $.post('/training/guess', data=request).then(
    function(data) {
      setStatus(data['status']);
      if (data['status'] == 'mismatch') {
        var li = $('<li>').text(data['argument'] + ' => ' + data['expected']);
        $('#evals').append(li);
      }
    },
    function(response) {
      setStatus(response.responseText);
    });
};
