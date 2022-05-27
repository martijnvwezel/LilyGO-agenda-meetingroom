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
  Later.setSeconds(Now.getSeconds() + 60 * 60 * 24*8);// next 24 hour, or days
  let events = Cal.getEvents(Now, Later);


  // only sent the following 8 events to reduce traffic
  let agenda_points = {};
  let max_events = 8;

  if (events.length < max_events) {
    max_events = events.length;
  }
  // convert events to json object
  for (var i = 0; i < events.length; i++) {
    let descrip = events[i].getDescription()
    descrip = descrip.replace(/<[^>]*>?/gm, ''); // remove html tags
    descrip = descrip.replace(/(\r\n|\n|\r)/gm, ""); // remove newlines
    descrip = descrip.replace(/\s\s+/g, ' '); // multipe space becomes one space, like taps newlines etc
    descrip = descrip.replace('Microsoft Teams meeting Join on your computer or mobile app Click here to join the meeting Learn More | Meeting options','');
    descrip = descrip.replace('Microsoft Teams-vergadering Deelnemen op uw computer of via de mobiele app Klik hier om deel te nemen aan de vergadering Meer informatie | Opties voor vergadering','');
    descrip = descrip.replace('Microsoft Teams-vergadering Neem deel via uw computer of mobiele app Klik hier om deel te nemen aan de vergadering Meer informatie | Opties voor vergadering','');
    descrip = descrip.replace(/^([a-zA-Z0-9]+\s)*[a-zA-Z0-9]+$/,'');
    descrip = replaceAll(descrip, '_', '');

    Logger.log(descrip);
    agenda_points[i] = {
      'title': events[i].getTitle().substring(0, 30),
      'description': descrip.substring(0, 800),
      'time': date_convert(events[i].getStartTime())+ '  ' + convert_time_string(events[i].getStartTime()) + ' - ' + convert_time_string(events[i].getEndTime())
    };
  }

  Logger.log(JSON.stringify(agenda_points));
  return JSON.stringify(agenda_points);

}

function replaceAll(string, search, replace) {
  return string.split(search).join(replace);
}

function padTo2Digits(num) {
  return String(num).padStart(2, '0');
}

function convert_time_string(time) {
  return padTo2Digits(time.getHours()) + ':' + padTo2Digits(time.getMinutes());
}
function date_convert(time){
  return padTo2Digits(time.getDate()) +'-'+ padTo2Digits(time.getMonth())+1;
}