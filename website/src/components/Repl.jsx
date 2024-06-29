import { createSignal, onCleanup, onMount } from "solid-js";
import "@kabelsalat/graphviz";
import { SalatRepl } from "@kabelsalat/core";
import { Icon } from "./Icon";

const defaultPatch = `// let's create some notes
let note = clock(150) // clock at 150bpm
.clockdiv(32) // divide clock by 32
.seq(64,0,0,67,0,0,62,0) // use clock for sequence

// use the notes to trigger an envelope
let env = note
.adsr(
  .01, // attack
  sine(.1).range(.1, .3), // modulated decay
  .5 // sustain
)
.mul(.75) // evelope amount

note
.hold(note) // hold notes above zeroes
.midinote() // convert midi numbers to freq
.pulse(.2) // pulse wave with .2 width
.filter(
  sine(.1).range(.7,.8).mul(env), // modulated cutoff
  // env.mul(.5) // resonance with envelope
)
.mul(env) // amplitude envelope
.add(
  // feedback delay
  x=>x.delay(sine(.03).range(.1,.5)) // modulated delay time
      .mul(.9) // feedback amount
)
.mul(.5) // master level
.out() // send to output`;

let vizSettings = { resolveModules: false, dagify: false };

export function Repl() {
  let [started, setStarted] = createSignal(false);
  const repl = new SalatRepl({ onToggle: (_started) => setStarted(_started) });

  function getURLCode() {
    let urlCode = window.location.hash.slice(1);
    if (urlCode) {
      urlCode = atob(urlCode);
    }
    return urlCode || "";
  }

  const initialCode = getURLCode() || defaultPatch;
  let [code, setCode] = createSignal(initialCode);
  let [hideCode, setHideCode] = createSignal(false);
  let container;
  async function run() {
    const node = repl.evaluate(code());
    node.render(container, vizSettings); // update viz
    window.location.hash = "#" + btoa(code());
    document.title = "🔌" + code().split("\n")[0];
    repl.play(node);
  }
  let handleKeydown = (e) => {
    // console.log("key", e.code);
    if (e.key === "Enter" && (e.ctrlKey || e.altKey)) {
      run();
    } else if (e.code === "Period" && (e.ctrlKey || e.altKey)) {
      repl.stop();
      e.preventDefault();
    } else if (e.key === "l" && e.ctrlKey) {
      setHideCode((hide) => !hide);
    }
  };
  let handlePopState = () => setCode(getURLCode());
  onMount(() => {
    document.addEventListener("keydown", handleKeydown);
    window.addEventListener("popstate", handlePopState);
  });
  onCleanup(() => {
    document.removeEventListener("keydown", handleKeydown);
    window.removeEventListener("popstate", handlePopState);
  });

  return (
    <div class="flex flex-col h-full max-h-full justify-stretch text-teal-600 font-mono">
      <div class="px-4 py-2 space-x-8 font-bold border-b border-stone-800 flex justify-between">
        <div
          class={`font-bold font-mono text-xl
        bg-gradient-to-r from-teal-400 to-fuchsia-300 inline-block text-transparent bg-clip-text
        `}
        >
          🔌 KABƎLSALAT
        </div>
        <div>
          <div class="flex justify-start items-center space-x-4 font-light">
            <button
              onClick={() => (started() ? repl.stop() : run())}
              class="items-center flex space-x-1"
            >
              {!started() ? (
                <>
                  <Icon type="play" />
                  <span class="animate-pulse">play</span>
                </>
              ) : (
                <>
                  <Icon type="stop" />
                  <span>stop</span>
                </>
              )}
            </button>
            <button onClick={() => run()} class="items-center flex space-x-1">
              <Icon type="refresh" />
              <span>update</span>
            </button>
            <a class="items-center flex space-x-1" href="/kabelsalat/learn">
              <Icon type="learn" />
              <span>learn</span>
            </a>
          </div>
        </div>
      </div>
      <div class="grid grid-cols-2 flex-auto shrink grow overflow-hidden">
        {!hideCode() && (
          <textarea
            class="bg-stone-900 shrink-0 p-4 focus:ring-0 outline-0 border-0"
            spellcheck="false"
            value={code()}
            onInput={(e) => setCode(e.target.value)}
          ></textarea>
        )}
        <div
          class={`bg-stone-900 overflow-auto text-gray-500 p-4 grow-0${
            hideCode() ? " col-span-2" : " border-l border-stone-800"
          }`}
          ref={(el) => {
            container = el;
            repl.evaluate(code()).render(container, vizSettings);
          }}
        ></div>
      </div>
      <div class="px-4 py-2 border-t border-stone-800 grow-0">
        <p>
          welcome to kabelsalat, is a very experimental audio graph live coding
          prototype.
        </p>
        <pre>keyboard: ctrl+enter: run, ctrl+dot: stop</pre>
      </div>
    </div>
  );
}
