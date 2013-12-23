// How to format dates.
var TIME_FMT = "ddd MMM DD YYYY hh:mm:ss A";
// Keycode of escape key.
var ESCAPE_KEYCODE = 27;

var win = $(window);
var doc = $(document);
var main = $("main");
var body = $("body");

var stars = $("#stars");
var cover = $("#cover");

var board = $("#board");
var boxes = $("#boxes");

var fileChooser = $("#fileChooser");
var bigButtonWrapper = $("#bigButtonWrapper");
var bigButton = $("#bigButton");

var pulldown = $("#pulldown");
var pulldownWrapper = $("#pulldownWrapper");
var pulldownDisplays = $(".pulldownDisplay");

var paramRoute = $("#paramRoute");
var paramDispatchTime = $("#paramDispatchTime");
var paramExpTime = $("#paramExpTime");
var paramBundleSize = $("#paramBundleSize");
var paramBundleClass = $("#paramBundleClass");

var navTabs = $("nav li");
var selectTabAnchor = $("#selectTab > a");

var routeTemplate = $("#routeTemplate");

// Create a clone of template.
function cloneTemplate(template) {
  return template.clone(true).removeAttr("id");
}

// Capitalize str.
function cap(str) {
  return str.charAt(0).toUpperCase() + str.slice(1);
}

function simplifySize(x) {
  var UNITS = ["B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"];
  var unit;

  unit = Math.floor(Math.log(x) / Math.log(1024));
  unit = Math.max(unit, 0);
  unit = Math.min(unit, UNITS.length - 1);

  x /= Math.pow(1024, unit);

  return x.toPrecision(3) + " " + UNITS[unit];
}

function hidePulldown() {
  navTabs.removeClass("selectedTab");
  pulldownDisplays.addClass("isHidden");
  pulldownWrapper.addClass("isHidden");
}

function blurBackground() {
  cover.show();
  main.addClass("blur");
}

function unblurBackground() {
  cover.hide();
  main.removeClass("blur");
}

function closeZoomBox() {
  var zoomBox = $("#zoomBox");

  if (!zoomBox[0])
    return;

  zoomBox.remove();
  unblurBackground();
}

// Build a box for a route.
function fillRoute(box, route, constants) {
  // Number of pixels to magnify the box.
  var MAGNIFY_PIXELS = 50;
  // Number of pixels to remove from all margins.
  var CONTRACT_PIXELS = -MAGNIFY_PIXELS / 2;

  var fromMoment = moment.unix(route.fromTime);
  var deliveryMoment = moment.unix(route.deliveryTime);

  var frame = $(".frame", box);
  var overlay = $(".overlay", box);
  var reason = $("p", box);
  var heading = $("h1", box);
  var graphDiv = $(".graph", box);
  var graph = $("img", box);

  $(".dispatchTime", box).html(fromMoment.format(TIME_FMT));
  $(".deliveryTime", box).html(deliveryMoment.format(TIME_FMT));
  $(".maxCapacity", box)
    .html(simplifySize(route.maxCapacity) +
      " (Payload Class " + (route.payloadClass + 1) + ")")
    .attr("title", route.maxCapacity + " bytes");

  // Wait until the image is loaded before using its dimensions, else they read
  // as zero.
  graph.attr("src", route.graph).one("load", function() {
    var imgHeight = graph[0].height;
    var divHeight = graphDiv.height();
    var divWidth = graphDiv.width();

    // If the image is shorter than the div, then clamp the div's height.
    if (imgHeight && imgHeight <= divHeight) {
      divHeight = imgHeight;
      overlay.hide();
    }

    graphDiv.height(divHeight);

    box
      .mouseenter(function(e) {
        graphDiv.width(divWidth + MAGNIFY_PIXELS);

        // If the image is taller than the div, then magnify on all sides. Else,
        // only magnify the width.
        if (imgHeight > divHeight) {
          graphDiv.height(divHeight + MAGNIFY_PIXELS);
          frame.css("margin", CONTRACT_PIXELS);
        } else {
          frame.css({
            "margin-left": CONTRACT_PIXELS,
            "margin-right": CONTRACT_PIXELS,
          });
        }
      })
      .mouseleave(function(e) {
        graphDiv.width("").height(divHeight);
        frame.css("margin", "");
      })
      .click(function(e) {
        var clone = $(this).clone(true);

        $(".frame", clone).css("margin", "");
        $(".graph", clone).width("").height("");

        blurBackground();

        clone
          .attr("id", "zoomBox")
          .css("top", window.pageYOffset)
          .off("mouseenter mouseleave click")
          .appendTo(body)
          .css("margin-left", -clone.width() / 2);

        cover
          .addClass("clickable")
          .one("click", function(e) {
            cover.removeClass("clickable");
          });
      });
  });

  if (route.flag == constants.SELECTED)
    reason.html("Best route found");
  else
    reason.html(cap(route.ignoreReason));

  switch (route.flag) {
  case constants.SELECTED:
    box.addClass("selectedRoute");
    heading.html("Selected");
  break;
  case constants.CONSIDERED:
    heading.html("Considered");
  break;
  case constants.IDENTIFIED:
    heading.html("Not Considered");
  break;
  default:
    heading.html("Ignored");
  break;
  }
}

// Build boxes for all routes.
function handleRoutes(routes, constants) {
  var selected = [];
  var considered = [];
  var identified = [];
  var ignored = [];

  $.each(routes, function(i, route) {
    var box = cloneTemplate(routeTemplate);
    fillRoute(box, route, constants);

    switch (route.flag) {
    case constants.SELECTED:
      selected.push(box);
    break;
    case constants.CONSIDERED:
      considered.push(box);
    break;
    case constants.IDENTIFIED:
      identified.push(box);
    break;
    default:
      ignored.push(box);
    break;
    }
  });

  $.each([selected, considered, identified, ignored], function() {
    $.each(this, function() {
      boxes.append(this);
    });
  });
}

// Fill in global parameters.
function handleParams(params) {
  var execMoment = moment.unix(params.execTime);
  var dispatchMoment = moment.unix(params.dispatchTime);
  var expMoment = moment.unix(params.expirationTime);

  paramRoute.html("Node " + params.localNode + " to Node " +
    params.destNode);
  paramDispatchTime.html(dispatchMoment.format(TIME_FMT));
  paramExpTime.html(expMoment.format(TIME_FMT));
  paramBundleSize
    .html(simplifySize(params.bundleSize))
    .attr("title", params.bundleSize + " bytes");
  paramBundleClass.html(params.minLatency ? "Minimum Latency"
                                          : "Best Effort");
}

function handleJSON(cgr) {
  handleParams(cgr.params);
  handleRoutes(cgr.routes, cgr.constants);
}

fileChooser.change(function(e) {
  var file = fileChooser.get(0).files[0];
  var reader = new FileReader();

  reader.addEventListener("load", function(e) {
    blurBackground();
    boxes.empty();

    handleJSON(JSON.parse(reader.result));

    win.off("resize").resize(function(e) {
      var boxDivs = $(".box", board);
      var width = boxDivs.outerWidth(true);

      // Snap boxes into columns depending on the width of the page.
      board.width(Math.max(Math.min(Math.floor(win.width() / width),
                                    boxDivs.length),
                           1) * width);
    }).resize().scrollTop(0);

    bigButtonWrapper.hide();
    pulldown.show();
    unblurBackground();
  });

  reader.readAsText(file);
})

$.each([
  ["#paramsTab", "#params"],
  ["#legendTab", "#legend"],
], function(i, pair) {
  var tab = $(pair[0]);
  var display = $(pair[1]);

  $("a", tab).click(function(e) {
    e.preventDefault();
    $(this).blur();

    if (tab.hasClass("selectedTab")) {
      hidePulldown();
    } else {
      navTabs.removeClass("selectedTab");
      tab.addClass("selectedTab");

      pulldownWrapper.removeClass("isPeeking");
      pulldownDisplays.addClass("isHidden");
      display.removeClass("isHidden");
      pulldownWrapper.removeClass("isHidden");
    }
  });
});

stars.click(hidePulldown);
cover.click(closeZoomBox);

doc.keydown(function(e) {
  if (e.keyCode == ESCAPE_KEYCODE)
    closeZoomBox();
});

$.each([bigButton, selectTabAnchor], function() {
  this.click(function(e) {
    e.preventDefault();
    fileChooser.click();
  });
});

pulldownDisplays.addClass("isHidden");
pulldown.hide();
blurBackground();
bigButton.focus();
