function doGet() {
  // Logger.log(ContentService.createTextOutput(GetEvents()));
  return ContentService.createTextOutput(GetEvents());
}

// Replace Muino with your calendar name

function GetEvents() {
  var _calendarName = 'Muino'
  let agenda_points = {};
  var Cal = CalendarApp.getCalendarsByName(_calendarName)[0];
  var Now = new Date();
  var Later = new Date();
  Later.setSeconds(Now.getSeconds() + 60 * 60 * 8);// next 8 hour
  // Logger.log(Now);
  Logger.log(Later);
  var events = Cal.getEvents(Now, Later);
  // Logger.log(events.length);
  str = "";
  Logger.log(events);



  for (var i = 0; i < events.length; i++) {
    agenda_points[i] = {
      'title': events[i].getTitle(),
      'getStartTime': events[i].getStartTime().getTime(),
      'getEndTime': events[i].getEndTime().getTime()
    };

  }

  // Logger.log(JSON.stringify(agenda_points));
  return JSON.stringify(agenda_points);

}