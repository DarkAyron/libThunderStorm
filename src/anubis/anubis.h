/**
 * <b>The Anubis block cipher.</b>
 *
 * <P>
 * <b>References</b>
 *
 * <P>
 * The Anubis algorithm was developed by
 * <a href="mailto:pbarreto@scopus.com.br">Paulo S. L. M. Barreto</a> and
 * <a href="mailto:vincent.rijmen@esat.kuleuven.ac.be">Vincent Rijmen</a>.
 *
 * See
 *		P.S.L.M. Barreto, V. Rijmen,
 *		``The Anubis block cipher,''
 *		NESSIE submission, 2000.
 * 
 * @author	Paulo S.L.M. Barreto
 * @author	Vincent Rijmen.
 * @author  Ayron
 *
 * @version 2.0 (2001.05.15)
 *
 * Differences from version 1.0:
 *
 * This software implements the "tweaked" version of Anubis.
 * Only the S-box and (consequently) the rounds constants have been changed.
 *
 * =============================================================================
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdint.h>

/* 
 * Anubis-specific definitions: 
 */ 

#define MIN_N			 4 
#define MAX_N			10 
#define MIN_ROUNDS		(8 + MIN_N) 
#define MAX_ROUNDS		(8 + MAX_N) 
#define MIN_KEYSIZEB	(4*MIN_N) 
#define MAX_KEYSIZEB	(4*MAX_N) 
#define BLOCKSIZE		128 
#define BLOCKSIZEB		(BLOCKSIZE/8) 

typedef struct anubisSchedule { 
	int keyBits; /* this field must be initialized before the NESSIEkeysetup call */ 
	int R; 
	uint32_t roundKeyEnc[MAX_ROUNDS + 1][4]; 
	uint32_t roundKeyDec[MAX_ROUNDS + 1][4]; 
} anubisSchedule_t;

/**
 * Create the Anubis key schedule for a given cipher key.
 * Both encryption and decryption key schedules are generated.
 * 
 * @param key			The 32N-bit cipher key.
 * @param structpointer	Pointer to the structure that will hold the expanded key.
 * @param keySize		Size of the key in bits.
 */
void anubisKeySetup(const unsigned char * key,
					anubisSchedule_t * structpointer,
				    int keySize);

/**
 * Encrypt a data block.
 * 
 * @param	structpointer	the expanded key.
 * @param	plaintext		the data block to be encrypted.
 * @param	ciphertext		the encrypted data block.
 */
void anubisEncrypt(const anubisSchedule_t * structpointer,
				   const unsigned char * plaintext,
				         unsigned char * ciphertext);
/**
 * Decrypt a data block.
 * 
 * @param	structpointer	the expanded key.
 * @param	ciphertext		the data block to be decrypted.
 * @param	plaintext		the decrypted data block.
 */
void anubisDecrypt(const anubisSchedule_t * structpointer,
				   const unsigned char * ciphertext,
				         unsigned char * plaintext);


