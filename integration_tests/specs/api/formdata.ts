describe('FormData', () => {
    it('should handle multiple appends of the same key', async () => {
      // 创建一个 FormData 实例
      const formData = new FormData();
      console.log('formData', formData, 'type=', formData.constructor.name);
  
      // 添加一些数据
      formData.append('key1', 'value1');
      formData.append('key1', 'value1.1');
      formData.append('key2', 'value2');
      formData.set('key2', 'new-value2');
  
      // 测试 get 方法
      console.log('Getting key1:', formData.get('key1'));
      expect(formData.get('key1')).toBe('value1.1'); // 最后一个值
  
      // 测试 getAll 方法
      console.log('Getting all key1:', formData.getAll('key1'));
      expect(formData.getAll('key1')).toEqual(['value1', 'value1.1']); // 所有值
  
      // 测试 has 方法
      console.log('Checking key1:', formData.has('key1'));
      console.log('Checking key2:', formData.has('key2'));
      expect(formData.has('key1')).toBe(true);
      expect(formData.has('key2')).toBe(true);
  
      // 测试 del 方法
      formData.delete('key1');
      console.log('Deleting key1:', formData.has('key1'));
      expect(formData.has('key1')).toBe(false);
  
      // 测试 forEach 方法
      const logEntries:string[] = [];
      formData.forEach((value, key) => {
        logEntries.push(`Iterating: Key: ${key}, Value: ${value}`);
      });
      console.log(logEntries.join('\n'));
      expect(logEntries).toContain('Iterating: Key: key2, Value: new-value2');
    });
  });