describe("calc", () => {
  it("001", async () => {
    let div;
    div = createElement(
      "div",
      {
        style: {
          width: "calc(100px +100px)",
          height: "calc(100px -50px)",
          backgroundColor: "green"
        }
      }, [createText("001")]
    );
    BODY.appendChild(div);
    await snapshot();
  });

  it("002", async () => {
    let div;
    div = createElement(
      "div",
      {
        style: {
          width: "calc(100px- 50px)",
          height: "calc(20px+ 30px)",
          backgroundColor: "green"
        }
      }, [createText("002")]
    );
    BODY.appendChild(div);
    await snapshot();
  });

  it("003", async () => {
    let div;
    div = createElement(
      "div",
      {
        style: {
          width: "calc(100px + 100px)",
          height: "calc(100px - 50px)",
          backgroundColor: "green"
        }
      }, [createText("003")]
    );
    BODY.appendChild(div);
    await snapshot();
  });

  it("004", async () => {
    let div;
    div = createElement(
      "div",
      {
        style: {
          width: "calc(20px*5)",
          height: "calc(300px/2)",
          backgroundColor: "green"
        }
      }, [createText("004")]
    );
    BODY.appendChild(div);
    await snapshot();
  });

  it("005", async () => {
    document.head.appendChild(
      createStyle(`
      :root {
        --num1: 20px;
        --num2: 20px;
      }
      .container {
        transform: translate(calc(var(--num1) + var(--num2)), 20px);
        background: green;
      }
    `)
    );

    document.body.appendChild(
      <div class='container'>
          <h2>The text should be green.</h2>
      </div>
    );
    await snapshot();
  });

  it('should works when clac same kind of length value', async () => {
    let box = createElement('div', {
      style: {
        width: 'calc(100vw - 50vw)',
        height: 'calc(100vh - 50vh)',
        background: 'green'
      }
    }, [
      createText('AAAAA')
    ]);
    document.body.appendChild(box);
    await snapshot();
  });

  it('should works when combine calc with vars', async () => {
    let box = createElement('div', {
      style: {
        width: 'calc(45* 1px * var(--base_size_ratio, 1))',
        height: 'calc(45* 1px * var(--base_size_ratio, 1))',
        background: 'green'
      }
    }, [
      createText('This box should be green')
    ]);
    document.body.appendChild(box);
    await snapshot();
  });

  function createStyle(text) {
    const style = document.createElement('style');
    style.appendChild(document.createTextNode(text));
    return style;
  }
});
