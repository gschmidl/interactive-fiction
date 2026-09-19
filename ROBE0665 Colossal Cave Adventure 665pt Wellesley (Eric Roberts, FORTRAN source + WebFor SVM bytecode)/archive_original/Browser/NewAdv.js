requirejs.config( { baseUrl: "js" });
requirejs([ "edu/stanford/cs/java2js",
            "edu/stanford/cs/webfor" ],

function(edu_stanford_cs_java2js,
         edu_stanford_cs_webfor) {

             let starterButton = document.getElementById("Starter");
             let wellesleyButton = document.getElementById("Wellesley");
             starterButton.addEventListener("click", startSmallAdventure);
             wellesleyButton.addEventListener("click", startBigAdventure);

             function startSmallAdventure() {
                 startGame(SMALL);
             }

             function startBigAdventure() {
                 startGame(BIG);
             }

             function startGame(code) {
                 let hourglass = document.getElementById("hourglass");
                 if (hourglass) hourglass.style.visibility = "visible";
                 let consoleWindow = document.getElementById("console");
                 if (consoleWindow) {
                     while (consoleWindow.firstChild) {
                         consoleWindow.removeChild(consoleWindow.firstChild);
                     }
                 }
                 let pgm = new edu_stanford_cs_webfor.WFProgram();
                 let tty = pgm.getConsole();
                 tty.setFont("Courier New-Bold-18");
                 tty.element.style.outline = "none";
                 tty.element.style.padding = "5px";
                 let svm = pgm.getSVM();
                 svm.setProgram(pgm);
                 svm.setCode(code);
                 setTimeout(runAdventure, 250);

                 function runAdventure() {
                     svm.run();
                 }
             }

          });
