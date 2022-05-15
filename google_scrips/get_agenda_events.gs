function doGet() {
  // Logger.log(ContentService.createTextOutput(GetEvents()));
  return ContentService.createTextOutput(GetEvents());
}

// Replace Muino with your calendar name

function GetEvents() {
  let _calendarName = 'Muino'

  // get events from agenda
  let Cal = CalendarApp.getCalendarsByName(_calendarName)[0];
  let Now = new Date();
  let Later = new Date();
  Later.setSeconds(Now.getSeconds() + 60 * 60 * 24);// next 24 hour
  let events = Cal.getEvents(Now, Later);


  // only sent the following 8 events to reduce traffic
  let agenda_points = {};
  let max_events = 16;

  if (events.length < max_events) {
    max_events = events.length;
  }
  // convert events to json object
  for (var i = 0; i < events.length; i++) {
    let descrip = events[i].getDescription()
    descrip = descrip.replace(/<[^>]*>?/gm, ''); // remove html tags
    descrip = descrip.substring(0, 80);
    agenda_points[i] = {
      'title': events[i].getTitle(),
      'description': descrip,
      'time': convert_time_string(events[i].getStartTime()) + ' - ' + convert_time_string(events[i].getEndTime())
    };

  }

  // Logger.log(JSON.stringify(agenda_points));
  return JSON.stringify(agenda_points);

}

function padTo2Digits(num) {
  return String(num).padStart(2, '0');
}

function convert_time_string(time) {
  return padTo2Digits(time.getHours()) + ':' + padTo2Digits(time.getMinutes());
}