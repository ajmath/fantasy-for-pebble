var REQUEST_TYPE_MATCHUP = 1;


function fetchCurrentMatchup(service, leagueId, teamId, callback) {
  var req = new XMLHttpRequest();
  // build the GET request
  req.open('GET', "https://fantasy-wrapper.herokuapp.com/currentmatchup/" + service + "?" + 
    "league=" + leagueId + "&team=" + teamId);
  req.onload = function(e) {
    if (req.readyState == 4) {
      // 200 - HTTP OK
      if(req.status == 200) {
        var response = JSON.parse(req.responseText);
        callback(response);
      }
    }
  };
  req.send(null);
}

function fetchMatchupData(service, leagueId, teamId) {
  fetchCurrentMatchup(service, leagueId, teamId, function(response){
    var pebble_msg = {
      "request_type": REQUEST_TYPE_MATCHUP,
      "team1_name"  : getTeam(response, teamId, 1).team_name,
      "team1_score" : cleanScore(getTeam(response, teamId, 1).score),
      "team2_name"  : getTeam(response, teamId, 2).team_name,
      "team2_score" : cleanScore(getTeam(response, teamId, 2).score),
      "team1_players" : getPlayers(getTeam(response, teamId, 1)),
      "team2_players" : getPlayers(getTeam(response, teamId, 2))
    };
    console.log("Sending score data to pebble: " + JSON.stringify(pebble_msg));
    Pebble.sendAppMessage(pebble_msg); 
  });
}

function getPlayers(team) {
  var playerstr = "";
  for (var i = 0; i < team.players.length; i++) {
    var p = team.players[i];
    playerstr = playerstr + p.name + ",";
    playerstr = playerstr + p.position + ",";
    playerstr = playerstr + p.team + ",";
    playerstr = playerstr + cleanScore(p.points) + "|";
  }
  return playerstr.substring(0, playerstr.length - 1);
}

function getTeam(response, teamId, teamIdx) {
  if(response.team1.team_id === teamId) {
    if(teamIdx == 1) {
      return response.team1; 
    } else if (teamIdx == 2) {
      return response.team2; 
    }
  }
  else {
    if(teamIdx == 1) {
      return response.team2; 
    } else if (teamIdx == 2) {
      return response.team1; 
    }
  }
  console.log("Couldnt find correct team");
  return response["team" + teamIdx];
}

function cleanScore(score) {
  return score.toFixed(2).replace(/\.?0*$/, "");
}

// Called when JS is ready
Pebble.addEventListener("ready", function(e) { });
												
// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage", function(e) {
  console.log("recieved message from pebble");
  console.log("payload is");
  showObjectContents(e.payload);
  if(e.payload.request_type == REQUEST_TYPE_MATCHUP) {
    fetchMatchupData("ff", e.payload.league_id, e.payload.team_id);
  } 
});

function showObjectContents(obj) {
  for(var prop in obj) {
    if (obj.hasOwnProperty(prop)) {
      console.log("obj." + prop + " = " + obj[prop]);
    }
  }
}