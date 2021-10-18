let el, eq

const renderEl = (str, el, displayMode, handleErr) => {
  window.katex.render(str, el, {
    throwOnError: handleErr,
    displayMode: displayMode
  })
}

const render = (str, el, displayMode=true, handleErr=false) => {
  if(window.katex) {
    if(el instanceof Array) {
      if(typeof str === "string") {
        el.forEach((e) => {
          console.log(e)
          renderEl(str, e, displayMode, handleErr)
        })
      } else {
        el.forEach((e, i) => {
          renderEl(str[i], e, displayMode, handleErr)
        })
      }
    } else if(str && el) {
      renderEl(str, el, displayMode, handleErr)
    } else {
      console.warn("Error, undefined el or string:", el, str)
    }
  } else setTimeout(() => { render(str, el, displayMode, handleErr) }, 50)
}

this.latex = { render: render }
