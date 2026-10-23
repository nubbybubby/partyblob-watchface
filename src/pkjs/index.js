var Clay = require('@rebble/clay');

new Clay([
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Time settings"
      },
      {
        "type": "toggle",
        "messageKey": "leading_zero",
        "label": "Don't show leading zero",
        "defaultValue": false
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Blob settings"
      },
      {
        "type": "text",
        "defaultValue": "Crank these settings up and watch him go!",
      },
      {
        "type": "slider",
        "messageKey": "blob_speed",
        "defaultValue": 1,
        "label": "Blob speed",
        "min": 0.25,
        "max": 2.5,
        "step": 0.05
      },
      {
        "type": "slider",
        "messageKey": "color_speed",
        "defaultValue": 1,
        "label": "Color speed",
        "min": 0.25,
        "max": 2.5,
        "step": 0.05
      },
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Bottom text"
      },
      {
        "type": "text",
        "defaultValue": "I'm going to be BRUTALly honest: I stole this. Read the strftime(3) man page for format specifiers.",
      },
      {
        "id": "bottom-select",
        "type": "select",
        "label": "Presets",
        "defaultValue": "%d, %b %d",
        "options": [
          { "value": "%a, %b %d",     "label": "DEFAULT: Fri, Sep 10" },
          { "value": "%d/%m/%y",      "label": "10/9/21" },
          { "value": "%Y.%m.%d",      "label": "2021.9.21" },
          { "value": "%d.%m.%Y",      "label": "10.9.2021" },
          { "value": "%A %d",         "label": "Friday 10" },
          { "value": "%a %d",         "label": "Fri 10" },
          { "value": "%B %d",         "label": "September 10" },
          { "value": "%b %d",         "label": "Sep 10" }
        ]
      },
      {
        "id": "bottom-input",
        "type": "input",
        "messageKey": "bottom_text",
        "defaultValue": "%a, %b %d"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save settings"
  }
], function () {
  this.on(this.EVENTS.AFTER_BUILD, function () {
    var bottomSelect = this.getItemById("bottom-select")
    var bottomInput = this.getItemById("bottom-input")

    bottomSelect.on("change", function () {
      bottomInput.set(bottomSelect.get())
    })
  }.bind(this))
})
