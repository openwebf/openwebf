/**
 * Test DOM API for
 * - document.createElement
 * - document.createTextNode
 * - document.createComment
 * - document.documentElement
 */
describe('Document api', () => {
  it('should work', async () => {
    const container = document.createElement('div');
    container.appendChild(document.createTextNode('This is a text node.'));
    container.appendChild(document.createComment('This is a comment'));
    document.body.appendChild(container);

    await snapshot();
  });

  it('documentElement', async () => {
    const documentElementWidth = document.documentElement.clientWidth;
    const documentElementHeight = document.documentElement.clientHeight;

    const text1 = document.createTextNode('documentElement width: ' + documentElementWidth + '\n');
    document.body.appendChild(text1);

    const text2 = document.createTextNode('documentElement height: ' + documentElementHeight + '\n');
    document.body.appendChild(text2);

    await snapshot();
  });

  it('document.all', () => {
    expect(document.all).not.toBeUndefined();
    expect(document.all.length).toBeGreaterThan(0);
  });

  it('document.domain', () => {
      expect(document.domain).not.toBeUndefined();
    });

  it('document.compatMode', () => {
      expect(document.compatMode).not.toBeUndefined();
    });

  it('document.readyState', () => {
      expect(document.readyState).not.toBeUndefined();
    });

  it('document.visibilityState', () => {
      expect(document.visibilityState).not.toBeUndefined();
    });

  it('document.hidden', () => {
      expect(document.hidden).not.toBeUndefined();
    });

  it('document.defaultView', () => {
    expect(document.defaultView).toBe(window);
  });

  it('document.elementFromPoint should work', async () => {
    const ele = document.createElement('div')
    ele.style.width = '100px';
    ele.style.height = '100px';
    ele.style.backgroundColor = 'blue';
    document.body.appendChild(ele);
    const findEle = document.elementFromPoint(50, 50);
    findEle.style.backgroundColor = 'yellow';
    await snapshot();
  });
});
