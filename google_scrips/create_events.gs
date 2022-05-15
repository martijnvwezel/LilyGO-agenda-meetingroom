function doGet(eve) {
  Logger.log(eve);
  let title = eve.parameter.title;
  return ContentService.createTextOutput(CreateEvent(title));
}

// Replace Muino with your calendar name

function CreateEvent(eve) {
  let _calendarName = 'Muino';
  let MILIS_PER_HOUR = 1000 * 60 * 15;
  let now = new Date();
  let from = new Date(now.getTime() - 60000);
  let to = new Date(now.getTime() + 1 * MILIS_PER_HOUR);
  let vCalendar = CalendarApp.getCalendarsByName(_calendarName)[0];

  vCalendar.createEvent(eve, from, to);
  return true;
}

