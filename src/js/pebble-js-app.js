var SCORE_WINDOW = 1;
var TEAM1_WINDOW = 2;
var TEAM2_WINDOW = 3;


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

function fetchScoreView(service, leagueId, teamId) {
  fetchCurrentMatchup(service, leagueId, teamId, function(response){
    var pebble_msg = {
      "window":SCORE_WINDOW,
      "team1_name"  : getTeam(response, teamId, 1).team_name,
      "team1_score" : cleanScore(getTeam(response, teamId, 1).score),
      "team2_name"  : getTeam(response, teamId, 2).team_name,
      "team2_score" : cleanScore(getTeam(response, teamId, 2).score)
    };
    console.log("Sending score data to pebble: " + JSON.stringify(pebble_msg));
    Pebble.sendAppMessage(pebble_msg); 
  });
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

function fetchTeamView(service, leagueId, teamId, whichTeam) {
  fetchCurrentMatchup(service, leagueId, teamId, function(response){
    console.log("fetching team view");
    sendTeamResponse(response[whichTeam]);
  });
}

function sendTeamResponse(team) {
  console.log("Sending response for " + team.team_name); 
}

// Called when JS is ready
Pebble.addEventListener("ready", function(e) { });
												
// Called when incoming message from the Pebble is received
Pebble.addEventListener("appmessage", function(e) {
  console.log("recieved message from pebble");
  console.log("payload is");
  showObjectContents(e.payload);
  if(e.payload.window == SCORE_WINDOW) {
    fetchScoreView("ff", e.payload.league_id, e.payload.team_id);
  } 
  else if (e.payload.window == TEAM1_WINDOW) {
    fetchTeamView("ff", e.payload.league_id, e.payload.team_id, "team1");
  } 
  else if (e.payload.window == TEAM2_WINDOW) {
    fetchTeamView("ff", e.payload.league_id, e.payload.team_id, "team2");
  }
});

function showObjectContents(obj) {
  for(var prop in obj) {
    if (obj.hasOwnProperty(prop)) {
      console.log("obj." + prop + " = " + obj[prop]);
    }
  }
}